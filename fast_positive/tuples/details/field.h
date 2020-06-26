/*
 *  Fast Positive Tuples (libfptu), aka Позитивные Кортежи
 *  Copyright 2016-2020 Leonid Yuriev <leo@yuriev.ru>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once
#include "fast_positive/erthink/erthink_casting.h"
#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/details/exceptions.h"
#include "fast_positive/tuples/details/utils.h"
#include "fast_positive/tuples/essentials.h"
#include "fast_positive/tuples/string_view.h"
#include "fast_positive/tuples/types/property.h"

#include <cstring>

#include "fast_positive/tuples/details/warnings_push_pt.h"

namespace fptu {
namespace details {

#pragma pack(push, 1)

struct field_loose;
class tuple_ro;

union stretchy_value_string {
  enum { tiny_threshold = 254u, max_length = UINT16_MAX * 2 + UINT8_MAX };
  struct {
    uint8_t length;
    char chars[3];
  } tiny;
  struct {
    uint8_t length_prefix;
    uint16_t length_suffix;
    char chars[1];
  } large;
  uint32_t pool_tag;
  unit_t flat[1];

  cxx11_constexpr bool is_pool_tag() const cxx11_noexcept {
    return tiny.length == 0 && unlikely(pool_tag > 0);
  }
  cxx11_constexpr bool is_tiny() const cxx11_noexcept {
    return tiny.length < tiny_threshold;
  }
  cxx11_constexpr std::size_t length() const cxx11_noexcept {
    return likely(is_tiny()) ? tiny.length
                             : large.length_prefix + large.length_suffix * 2u;
  }
  cxx11_constexpr std::size_t brutto_units() const cxx11_noexcept {
    return bytes2units((is_tiny()) ? 1u + tiny.length
                                   : 3u + large.length_prefix +
                                         large.length_suffix * 2u);
  }

  static std::size_t estimate_space(const std::size_t string_length) {
    if (unlikely(string_length > max_length))
      throw_value_too_long();
    return bytes2units(string_length +
                       ((string_length < tiny_threshold) ? 1u : 3u));
  }
  static std::size_t estimate_space(const string_view &value) {
    return estimate_space(value.size());
  }

  void store(const string_view &value) {
    const std::size_t string_length = value.size();
    // clear the last unit
    flat[bytes2units(string_length +
                     ((string_length < tiny_threshold) ? 1u : 3u)) -
         1] = 0;
    tiny.length = uint8_t(string_length);
    char *place = tiny.chars;
    if (string_length >= tiny_threshold) {
      assert(string_length <= max_length);
      large.length_prefix = uint8_t(tiny_threshold + (string_length & 1u));
      large.length_suffix =
          uint16_t((string_length - large.length_prefix) >> 1);
      place = large.chars;
    }
    assert(string_length == length());
    std::memcpy(place, value.data(), string_length);
    assert(brutto_units() == estimate_space(value));
    assert(estimate_space(string_length + 1) > brutto_units() ||
           begin()[string_length] == '\0');
  }

  cxx11_constexpr const char *begin() const cxx11_noexcept {
    static_assert(sizeof(*this) == sizeof(unit_t), "Oops");
    return likely(is_tiny()) ? tiny.chars : large.chars;
  }
  cxx11_constexpr const char *end() const cxx11_noexcept {
    return begin() + length();
  }

  stretchy_value_string() = delete;
  ~stretchy_value_string() = delete;
};

union stretchy_value_varbin {
  struct {
    uint16_t brutto_units;
    uint16_t reserved14_tailbytes;
  };
  unit_t flat[1];

  cxx14_constexpr std::size_t length() const cxx11_noexcept {
    static_assert(8 - genus_bitness == 3, "Oops");
    const std::size_t tailbytes = reserved14_tailbytes & 3;
    constexpr_assert(brutto_units > 1 || tailbytes == 0);
    return units2bytes(brutto_units) - 4 + tailbytes;
  }

  static std::size_t estimate_space(const string_view &value) {
    const std::size_t brutto = bytes2units(value.length()) + 1;
    if (unlikely(brutto > UINT16_MAX))
      throw_value_too_long();
    return brutto;
  }

  void store(const string_view &value) {
    const std::size_t value_length = value.size();
    const std::size_t brutto = bytes2units(value_length) + 1;
    assert(brutto <= UINT16_MAX);
    // clear the last unit
    flat[brutto - 1] = 0;
    static_assert(sizeof(unit_t) - 1 == 3, "Oops");
    static_assert(sizeof(*this) == sizeof(unit_t), "Oops");
    brutto_units = uint16_t(brutto);
    reserved14_tailbytes = value_length & 3;
    std::memcpy(flat + 1, value.data(), value_length);
    assert(value.length() == length());
    assert(brutto_units == estimate_space(value));
  }

  cxx11_constexpr const void *begin() const cxx11_noexcept {
    static_assert(sizeof(*this) == sizeof(unit_t), "Oops");
    return erthink::constexpr_pointer_cast<const char *>(this) + sizeof(*this);
  }
  cxx14_constexpr const void *end() const cxx11_noexcept {
    return static_cast<const char *>(begin()) + length();
  }

  stretchy_value_varbin() = delete;
  ~stretchy_value_varbin() = delete;
};

struct stretchy_value_tuple {
  union {
    /* См описание ограничений и компромиссов в комментариях к типу enum
     * fptu::fundamentals в essentials.h */
    struct {
      uint16_t brutto_units;
      uint16_t looseitems_flags;
    };
    unit_t flat[1];
  };
  enum {
    flags_bits = 3,
    sorted_flag = 1,
    preplaced_flag = 2,
    reserved_flag = 4
  };

  cxx11_constexpr std::size_t length() const cxx11_noexcept {
    return units2bytes(brutto_units);
  }
  void set_index_size_and_flags(ptrdiff_t count, unsigned flags) {
    assert(count >= 0 && count <= max_fields);
    assert(flags < (1u << flags_bits));
    looseitems_flags = uint16_t((count << flags_bits) + flags);
  }
  cxx11_constexpr std::size_t index_size() const cxx11_noexcept {
    static_assert(int(fundamentals::tuple_flags_bits) == int(flags_bits),
                  "WTF?");
    static_assert(sizeof(*this) == sizeof(unit_t), "Oops");
    return looseitems_flags >> flags_bits;
  }
  cxx11_constexpr bool is_sorted() const cxx11_noexcept {
    return (looseitems_flags & sorted_flag) != 0;
  }
  cxx11_constexpr bool have_preplaced() const cxx11_noexcept {
    return (looseitems_flags & preplaced_flag) != 0;
  }

  static std::size_t estimate_space(const stretchy_value_tuple *value) {
    assert(value == nullptr || value->brutto_units > 0);
    return value ? value->brutto_units : 0;
  }

  void store(const stretchy_value_tuple *const value) {
    assert(value != this);
    brutto_units = 0;
    looseitems_flags = 0;
    if (likely(value)) {
      assert(value->brutto_units > 0);
      std::memcpy(flat, value, units2bytes(value->brutto_units));
    }
  }

  cxx11_constexpr bool is_hollow() const cxx11_noexcept {
    return brutto_units < 1;
  }
  cxx11_constexpr const void *pivot() const cxx11_noexcept;

  cxx11_constexpr const field_loose *begin_index() const cxx11_noexcept {
    static_assert(sizeof(*this) == sizeof(unit_t), "Oops");
    return erthink::constexpr_pointer_cast<const field_loose *>(this + 1);
  }
  cxx11_constexpr const field_loose *end_index() const cxx11_noexcept {
    return erthink::constexpr_pointer_cast<const field_loose *>(pivot());
  }
  cxx11_constexpr const unit_t *begin_data_units() const cxx11_noexcept {
    return erthink::constexpr_pointer_cast<const unit_t *>(pivot());
  }
  cxx11_constexpr const unit_t *end_data_units() const cxx11_noexcept {
    return erthink::constexpr_pointer_cast<const unit_t *>(this) + brutto_units;
  }
  cxx14_constexpr std::size_t payload_units() const cxx11_noexcept {
    const ptrdiff_t result = brutto_units - 1 - index_size();
    constexpr_assert(result >= 0 &&
                     result == end_data_units() - begin_data_units());
    return result;
  }
  cxx11_constexpr const char *begin_data_bytes() const cxx11_noexcept {
    return erthink::constexpr_pointer_cast<const char *>(begin_data_units());
  }
  cxx11_constexpr const char *end_data_bytes() const cxx11_noexcept {
    return erthink::constexpr_pointer_cast<const char *>(end_data_units());
  }
  cxx14_constexpr std::size_t payload_bytes() const cxx11_noexcept {
    const ptrdiff_t result = end_data_bytes() - begin_data_bytes();
    constexpr_assert(result >= 0 &&
                     size_t(result) == units2bytes(payload_units()));
    return result;
  }

  stretchy_value_tuple() = delete;
  ~stretchy_value_tuple() = delete;
};

union stretchy_value_property {
  struct {
    uint8_t data_length;
    uint8_t id;
    uint8_t bytes[2];
  };
  unit_t flat[1];

  cxx11_constexpr std::size_t whole_length() const cxx11_noexcept {
    return 2 + data_length;
  }

  cxx11_constexpr std::size_t brutto_units() const cxx11_noexcept {
    static_assert(sizeof(*this) == sizeof(unit_t), "Oops");
    return bytes2units(whole_length());
  }

  static std::size_t estimate_space(const property_pair &value) {
    const std::size_t value_length = value.first.size();
    if (value_length > UINT8_MAX)
      throw_value_too_long();
    return bytes2units(value_length + 2);
  }

  void store(const property_pair &value) {
    const std::size_t value_length = value.first.size();
    assert(value_length <= UINT8_MAX);
    // clear the last unit
    flat[bytes2units(value_length + 2) - 1] = 0;
    data_length = uint8_t(value_length);
    id = value.second;
    std::memcpy(this->bytes, value.first.data(), value_length);
    assert(brutto_units() == estimate_space(value));
  }

  stretchy_value_property() = delete;
  ~stretchy_value_property() = delete;
};

union relative_payload {
  union {
    stretchy_value_string string;
    stretchy_value_varbin varbin;
    stretchy_value_tuple tuple;
    stretchy_value_property property;

    cxx14_constexpr std::size_t brutto_units(genus type) const {
      constexpr_assert(!is_fixed_size(type));
      switch (type) {
      default:
        constexpr_assert(false);
        __unreachable();
        return 0;
      case genus::text:
        return string.brutto_units();
      case genus::varbin:
        return varbin.brutto_units;
      case genus::nested:
        return tuple.brutto_units;
      case genus::property:
        return property.brutto_units();
      }
    }

    cxx14_constexpr std::size_t length(genus type) const {
      constexpr_assert(!is_fixed_size(type));
      switch (type) {
      default:
        constexpr_assert(false);
        __unreachable();
        return 0;
      case genus::text:
        return string.length();
      case genus::varbin:
        return varbin.length();
      case genus::nested:
        return tuple.length();
      case genus::property:
        return property.whole_length();
      }
    }
  } stretchy;

  union {
    uint32_t u32;
    int32_t i32;
    float f32;
    uint64_t unaligned_u64;
    int64_t unaligned_i64;
    double unaligned_f64;
    char bytes[8];
    uint32_t words[16];
  } fixed;

  unit_t flat[1];

  relative_payload() = delete;
  ~relative_payload() = delete;
};

struct relative_offset {
  uint16_t offset_uint16;

  cxx14_constexpr void add_delta(const ptrdiff_t delta) cxx11_noexcept {
    constexpr_assert(delta >= -UINT16_MAX && delta <= UINT16_MAX);
    const ptrdiff_t full_offset = offset_uint16 + delta;
    constexpr_assert(full_offset > 0 && full_offset <= UINT16_MAX);
    offset_uint16 = static_cast<uint16_t>(full_offset);
  }
  cxx14_constexpr void sub_delta(const ptrdiff_t delta) cxx11_noexcept {
    add_delta(-delta);
  }
  cxx11_constexpr bool have_payload() const cxx11_noexcept {
    return offset_uint16 > 0;
  }
  cxx14_constexpr unit_t *base() cxx11_noexcept {
    return erthink::constexpr_pointer_cast<unit_t *>(this);
  }

  cxx14_constexpr relative_payload *payload() cxx11_noexcept {
    constexpr_assert(have_payload());
    return erthink::constexpr_pointer_cast<relative_payload *>(base() +
                                                               offset_uint16);
  }
  cxx14_constexpr const relative_payload *payload() const cxx11_noexcept {
    return const_cast<relative_offset *>(this)->payload();
  }

  cxx14_constexpr void set_payload(const void *payload) cxx11_noexcept {
    constexpr_assert(payload != nullptr);
    const ptrdiff_t ptrdiff =
        erthink::constexpr_pointer_cast<const unit_t *>(payload) - base();
    constexpr_assert(ptrdiff > 0 && ptrdiff <= UINT16_MAX);
    offset_uint16 = static_cast<uint16_t>(ptrdiff);
  }
  cxx14_constexpr void reset_payload() cxx11_noexcept { offset_uint16 = 0; }

  relative_offset() = delete;
  relative_offset(const relative_offset &) = delete;
  relative_offset &operator=(const relative_offset &) = delete;
};

union field_preplaced {
  char bytes[1];
  relative_offset relative;

  FPTU_API bool is_null(tag_t tag) const cxx11_noexcept;
  field_preplaced() = delete;
  ~field_preplaced() = delete;
};

struct field_loose {
  using inplace_storage_t = int16_t;
  union {
    struct {
      union {
        relative_offset relative;
        inplace_storage_t inplaced;
      };
      loose_genus_and_id_t /* uint16_t */ genus_and_id;
    };
    uint32_t loose_header;
  };

  cxx11_constexpr genus type() const { return descriptor2genus(genus_and_id); }
  cxx11_constexpr bool is_hole() const { return type() == hole; }
  cxx11_constexpr unsigned id() const {
    constexpr_assert(!is_hole());
    return descriptor2id(genus_and_id);
  }
  cxx11_constexpr unsigned hole_get_units() const {
    constexpr_assert(is_hole());
    return descriptor2id(genus_and_id);
  }
  void hole_set_units(size_t units) {
    genus_and_id = loose_genus_and_id_t(details::make_hole(units));
  }
  cxx14_constexpr const unit_t *hole_begin() const {
    constexpr_assert(is_hole() && hole_get_units() > 0);
    return relative.payload()->flat;
  }
  const unit_t *hole_end() const { return hole_begin() + hole_get_units(); }
  cxx11_constexpr std::size_t stretchy_units() const cxx11_noexcept {
    return relative.have_payload()
               ? relative.payload()->stretchy.brutto_units(type())
               : 0;
  }
  void hole_purge() {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    loose_header = details::make_hole(0) << 16;
#else
    loose_header = loose_genus_and_id_t(details::make_hole(0));
#endif
    assert(hole_get_units() == 0 && !relative.have_payload());
  }

  field_loose() = delete;
  ~field_loose() = delete;
};

cxx11_constexpr const void *stretchy_value_tuple::pivot() const cxx11_noexcept {
  static_assert(sizeof(tag_t) == sizeof(field_loose), "WTF?");
  return begin_index() + index_size();
}

class preplaced_stretchy_value : protected relative_offset {
public:
  cxx11_constexpr bool nil() const cxx11_noexcept {
    return !relative_offset::have_payload();
  }
};

#pragma pack(pop)

} // namespace details

class preplaced_string;
class preplaced_varbin;
class preplaced_nested;
class preplaced_property;

} // namespace fptu

#include "fast_positive/tuples/details/warnings_pop.h"
