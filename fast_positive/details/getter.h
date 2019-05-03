/*
 * Copyright 2016-2019 libfptu authors: please see AUTHORS file.
 *
 * This file is part of libfptu, aka "Fast Positive Tuples".
 *
 * libfptu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libfptu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libfptu.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "fast_positive/details/api.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/exceptions.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/legacy_common.h"
#include "fast_positive/details/meta.h"
#include "fast_positive/details/scan.h"
#include "fast_positive/details/schema.h"
#include "fast_positive/details/token.h"

#include <iterator>

#include "fast_positive/details/warnings_push_pt.h"

namespace fptu {
namespace details {

template <typename> class crtp_getter;
template <typename> class accessor_ro;
template <typename> class iterator_ro;
template <typename> class collection_ro;
class tuple_ro;
class tuple_rw;

struct iovec_thunk : public iovec {
  iovec_thunk() = default;
  iovec_thunk(const void *data, std::size_t size) : iovec(data, size) {}
  iovec_thunk(const string_view &view) : iovec(view.data(), view.size()) {}
  iovec_thunk(const stretchy_value_tuple *nested)
      : iovec(nested, nested ? nested->length() : 0) {}

  operator const uint8_t *() const {
    return static_cast<const uint8_t *>(iov_base);
  }
  operator string_view() const {
    return string_view(static_cast<const char *>(iov_base), iov_len);
  }

  inline iovec_thunk(const tuple_ro *ro);

  iovec_thunk(const fptu_ro &ro) : iovec(ro.sys) {}
  operator fptu_ro() const {
    fptu_ro result;
    result.sys = *this;
    return result;
  }
};

template <genus GENUS, typename erthink::enable_if_t<
                           meta::genus_traits<GENUS>::physique !=
                           meta::physique_kind::stretchy> * = nullptr>
inline cxx14_constexpr typename meta::genus_traits<GENUS>::return_type
get(const field_loose *loose, const bool quietabsence) {
  if (likely(loose)) {
    if (unlikely(loose->type() != GENUS))
      throw_type_mismatch();
    return meta::genus_traits<GENUS>::read(loose);
  }
  if (likely(quietabsence))
    return meta::genus_traits<GENUS>::empty();
  throw_field_absent();
}

template <genus GENUS, typename erthink::enable_if_t<
                           meta::genus_traits<GENUS>::physique ==
                           meta::physique_kind::stretchy> * = nullptr>
inline cxx14_constexpr typename meta::genus_traits<GENUS>::return_type
get(const field_loose *loose, const bool quietabsence) {
  if (likely(loose)) {
    if (unlikely(loose->type() != GENUS))
      throw_type_mismatch();
    if (likely(loose->relative.have_payload()))
      return meta::genus_traits<GENUS>::read(loose->relative.payload());
  }
  if (likely(quietabsence))
    return meta::genus_traits<GENUS>::empty();
  throw_field_absent();
}

template <typename TOKEN> class accessor_ro {
protected:
  combo_ptr field_;
  TOKEN token_;
  template <typename> friend class iterator_ro;
  template <typename> friend class crtp_getter;
  friend class tuple_rw;

  template <genus GENUS, typename erthink::enable_if_t<
                             meta::genus_traits<GENUS>::physique !=
                             meta::physique_kind::stretchy> * = nullptr>
  inline cxx14_constexpr typename meta::genus_traits<GENUS>::return_type
  get() const {
    if (unlikely(token_.type() != GENUS))
      throw_type_mismatch();

    if (likely(token_.is_preplaced())) {
      // preplaced
      if (likely(!token_.distinct_null() ||
                 !meta::genus_traits<GENUS>::is_denil(field_.preplaced)))
        return meta::genus_traits<GENUS>::read(field_.preplaced);
    } else if (likely(field_.loose)) {
      // loose exist
      assert(tag2genus(field_.loose->genius_and_id) == GENUS);
      return meta::genus_traits<GENUS>::read(field_.loose);
    }

    // preplaced denil or loose absence
    if (likely(token_.is_quietabsence()))
      return meta::genus_traits<GENUS>::empty();
    throw_field_absent();
  }

  template <genus GENUS, typename erthink::enable_if_t<
                             meta::genus_traits<GENUS>::physique ==
                             meta::physique_kind::stretchy> * = nullptr>
  inline cxx14_constexpr typename meta::genus_traits<GENUS>::return_type
  get() const {
    if (unlikely(token_.type() != GENUS))
      throw_type_mismatch();

    if (likely(token_.is_preplaced() || field_.loose)) {
      assert(token_.is_preplaced() ||
             tag2genus(field_.loose->genius_and_id) == GENUS);
      static_assert(offsetof(field_preplaced, relative) ==
                        offsetof(field_loose, relative),
                    "WTF?");
      const auto &relative = field_.relative_reference();
      if (likely(relative.have_payload()))
        return meta::genus_traits<GENUS>::read(relative.payload());
    }

    // loose field or value is absent
    if (likely(token_.is_quietabsence() || !token_.distinct_null()))
      return meta::genus_traits<GENUS>::empty();
    throw_field_absent();
  }

  constexpr accessor_ro() : field_((void *)(nullptr)), token_() {
    static_assert(sizeof(*this) <= (TOKEN::is_static_token::value
                                        ? sizeof(void *)
                                        : sizeof(void *) * 2),
                  "WTF?");
  }

  constexpr accessor_ro(const field_preplaced *target,
                        const TOKEN token) noexcept
      : field_(target), token_(token) {}

  cxx14_constexpr accessor_ro(const field_loose *target,
                              const field_loose *detent,
                              const TOKEN token) noexcept
      : field_(target), token_(token) {
    constexpr_assert(target < detent);
    (void)detent;
  }

public:
  constexpr accessor_ro(const accessor_ro &) noexcept = default;
  cxx14_constexpr accessor_ro &
  operator=(const accessor_ro &) noexcept = default;
  constexpr accessor_ro(accessor_ro &&) noexcept = default;
  cxx14_constexpr accessor_ro &operator=(accessor_ro &&) noexcept = default;

  constexpr bool exist() const noexcept {
    return token_.is_preplaced() ? !token_.distinct_null() ||
                                       field_.preplaced->is_denil(token_.tag())
                                 : field_.loose != nullptr;
  }
  constexpr operator bool() const noexcept { return exist(); }

  constexpr genus type() const noexcept { return token_.type(); }
  constexpr const TOKEN &token() const noexcept { return token_; }
  constexpr operator TOKEN() const noexcept { return token_; }

  constexpr string_view get_string() const { return get<text>(); }
  constexpr string_view get_varbinary() const { return get<varbin>(); }
  constexpr const tuple_ro *get_nested() const {
    return erthink::constexpr_pointer_cast<const tuple_ro *>(get<nested>());
  }
  constexpr property_pair get_property() const { return get<property>(); }
  constexpr bool get_bool() const { return get<boolean>() != 0; }
  constexpr short get_enum() const { return get<enumeration>(); }
  constexpr int8_t get_i8() const { return get<i8>(); }
  constexpr uint8_t get_u8() const { return get<u8>(); }
  constexpr int16_t get_i16() const { return get<i16>(); }
  constexpr uint16_t get_u16() const { return get<u16>(); }
  constexpr int32_t get_i32() const { return get<i32>(); }
  constexpr uint32_t get_u32() const { return get<u32>(); }
  constexpr int64_t get_i64() const { return get<i64>(); }
  constexpr uint64_t get_u64() const { return get<u64>(); }
  constexpr float get_f32() const { return get<f32>(); }
  constexpr double get_f64() const { return get<f64>(); }
  constexpr decimal64 get_decimal() const { return get<d64>(); }
  constexpr datetime_t get_datetime() const { return get<t64>(); }
  constexpr const uuid_t &get_uuid() const {
    return *erthink::constexpr_pointer_cast<const uuid_t *>(&get<b128>());
  }

  constexpr const binary96_t &get_b96() const {
    return *erthink::constexpr_pointer_cast<const binary96_t *>(&get<b96>());
  }
  constexpr const binary128_t &get_b128() const {
    return *erthink::constexpr_pointer_cast<const binary128_t *>(&get<b128>());
  }
  constexpr const binary160_t &get_b160() const {
    return *erthink::constexpr_pointer_cast<const binary160_t *>(&get<b160>());
  }
  constexpr const binary192_t &get_b192() const {
    return *erthink::constexpr_pointer_cast<const binary192_t *>(&get<b192>());
  }
  constexpr const binary224_t &get_b224() const {
    return *erthink::constexpr_pointer_cast<const binary224_t *>(&get<b224>());
  }
  constexpr const binary256_t &get_b256() const {
    return *erthink::constexpr_pointer_cast<const binary256_t *>(&get<b256>());
  }
  constexpr const binary320_t &get_b320() const {
    return *erthink::constexpr_pointer_cast<const binary320_t *>(&get<b320>());
  }
  constexpr const binary384_t &get_b384() const {
    return *erthink::constexpr_pointer_cast<const binary384_t *>(&get<b384>());
  }
  constexpr const binary512_t &get_b512() const {
    return *erthink::constexpr_pointer_cast<const binary512_t *>(&get<b512>());
  }

  constexpr const ip_address_t &get_ip_address() const {
    return *erthink::constexpr_pointer_cast<const ip_address_t *>(&get<ip>());
  }
  constexpr mac_address_t get_mac_address() const { return get<mac>(); }
  constexpr const ip_net_t &get_ip_net() const {
    return *erthink::constexpr_pointer_cast<const ip_net_t *>(&get<ipnet>());
  }

  constexpr double get_float() const {
    return (type() == f32) ? get_f32() : get_f64();
  }

  cxx14_constexpr int64_t get_integer() const {
    switch (type()) {
    default:
      throw_type_mismatch();
    case i8:
      return get_i8();
    case i16:
      return get_i16();
    case i32:
      return get_i32();
    case i64:
      return get_i64();
    }
  }

  cxx14_constexpr uint64_t get_unsigned() const {
    switch (type()) {
    default:
      throw_type_mismatch();
    case u8:
      return get_u8();
    case u16:
      return get_u16();
    case u32:
      return get_u32();
    case u64:
      return get_u64();
    }
  }

  cxx14_constexpr int128_t get_int128() const {
    if (type() == b128)
      return *erthink::constexpr_pointer_cast<const int128_t *>(&get<b128>());
    else
      return int128_t(get_integer());
  }
  cxx14_constexpr uint128_t get_uint128() const {
    if (type() == b128)
      return *erthink::constexpr_pointer_cast<const uint128_t *>(&get<b128>());
    else
      return uint128_t(get_unsigned());
  }
};

template <typename TOKEN>
class iterator_ro
    : protected accessor_ro<TOKEN>
#if __cplusplus < 201703L
    ,
      public std::iterator<std::forward_iterator_tag, const accessor_ro<TOKEN>,
                           std::ptrdiff_t, const accessor_ro<TOKEN> *,
                           const accessor_ro<TOKEN> &>
#endif
{
  const field_loose *detent_;

protected:
  template <typename> friend class collection_ro;
  friend class tuple_rw;
  using accessor = accessor_ro<TOKEN>;

  explicit constexpr iterator_ro(const field_loose *target,
                                 const field_loose *detent,
                                 const TOKEN token) noexcept
      : accessor(target, detent, token), detent_(detent) {
    constexpr_assert(token.is_collection());
  }

public:
#if __cplusplus >= 201703L
  using iterator_category = std::forward_iterator_tag;
  using value_type = const accessor_ro<TOKEN>;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using reference = value_type &;
#endif

  constexpr iterator_ro() noexcept : detent_(accessor::field_.loose) {
    static_assert(sizeof(*this) <= (TOKEN::is_static_token::value
                                        ? sizeof(void *) * 2
                                        : sizeof(void *) * 3),
                  "WTF?");
  }
  constexpr iterator_ro(const iterator_ro &) noexcept = default;
  cxx14_constexpr iterator_ro &
  operator=(const iterator_ro &) noexcept = default;
  constexpr const TOKEN &token() const noexcept { return accessor::token(); }

  iterator_ro &operator++() noexcept {
    assert(accessor::field_.loose < detent_);
    accessor::field_.const_loose =
        next(accessor::field_.const_loose, detent_, accessor::token_.tag());
    return *this;
  }
  iterator_ro operator++(int) const noexcept {
    iterator_ro iterator(*this);
    ++iterator;
    return iterator;
  }
  constexpr const accessor &operator*() const noexcept { return *this; }

  constexpr bool operator==(const accessor &other) const noexcept {
    return accessor::field_.ptr == other.field_.ptr;
  }
  constexpr bool operator!=(const accessor &other) const noexcept {
    return accessor::field_.ptr != other.field_.ptr;
  }
  constexpr bool operator==(const iterator_ro &other) const noexcept {
    return accessor::field_.ptr == other.field_.ptr;
  }
  constexpr bool operator!=(const iterator_ro &other) const noexcept {
    return accessor::field_.ptr != other.field_.ptr;
  }
};

template <typename TOKEN> class collection_ro {
  template <typename> friend class crtp_getter;
  friend class tuple_rw;

  iterator_ro<TOKEN> iterator_;

  constexpr collection_ro(const field_loose *first, const field_loose *detent,
                          const TOKEN token) noexcept
      : iterator_(first, detent, token) {
    constexpr_assert(token.is_collection());
  }

public:
  using const_iterator = iterator_ro<TOKEN>;
  constexpr collection_ro(const collection_ro &) = default;
  cxx14_constexpr collection_ro &operator=(const collection_ro &) = default;
  constexpr const TOKEN &token() const noexcept { return iterator_.token(); }

  constexpr const const_iterator &begin() const noexcept { return iterator_; }
  constexpr const_iterator end() const noexcept {
    return const_iterator(nullptr, iterator_.detent_, iterator_.token_);
  }
  constexpr bool empty() const noexcept { return begin() == end(); }
};

} // namespace details

using dynamic_accessor_ro = details::accessor_ro<token>;
using dynamic_iterator_ro = details::iterator_ro<token>;
using dynamic_collection_ro = details::collection_ro<token>;

namespace details {

template <typename TUPLE> class crtp_getter {
  friend class tuple_rw;

  constexpr const TUPLE &self() const {
    return *static_cast<const TUPLE *>(this);
  }

  template <typename TOKEN, bool ALLOW_COLLECTION = false, typename CRTP_TUPLE>
  static __pure_function accessor_ro<TOKEN> at(const CRTP_TUPLE &tuple,
                                               const TOKEN &token) {
    if (likely(token.is_preplaced())) {
      const std::size_t offset = token.preplaced_offset();
      constexpr_assert(tuple.have_preplaced() &&
                       offset < max_tuple_bytes_netto);
      constexpr_assert(tuple.begin_data_bytes() + offset <
                       tuple.end_data_bytes());
      const field_preplaced *const target =
          erthink::constexpr_pointer_cast<const field_preplaced *>(
              tuple.begin_data_bytes() + offset);
      return accessor_ro<TOKEN>(target, token);
    }

    if (!ALLOW_COLLECTION && unlikely(token.is_collection()))
      throw_collection_unallowed();
    const field_loose *const detent = tuple.end_index();
    const field_loose *const first =
        lookup(tuple.is_sorted(), tuple.begin_index(), detent, token.tag());
    return accessor_ro<TOKEN>(first, detent, token);
  }

  template <typename TOKEN, typename CRTP_TUPLE>
  static __pure_function collection_ro<TOKEN> collect(const CRTP_TUPLE &tuple,
                                                      const TOKEN &token) {
    if (unlikely(!token.is_collection()))
      throw_collection_required();

    const field_loose *const detent = tuple.end_index();
    const field_loose *const first =
        lookup(tuple.is_sorted(), tuple.begin_index(), detent, token.tag());
    return collection_ro<TOKEN>(first, detent, token);
  }

  template <typename TOKEN, typename CRTP_TUPLE>
  static __pure_function bool have(const CRTP_TUPLE &tuple,
                                   const TOKEN &token) {
    return token.is_collection() ? !collect(tuple, token).empty()
                                 : at(tuple, token).exist();
  }

public:
#define HERE_CRTP_MAKE(RETURN_TYPE, NAME)                                      \
  FPTU_TEMPLATE_FOR_STATIC_TOKEN                                               \
  __pure_function inline RETURN_TYPE NAME(const TOKEN &ident) const {          \
    return at(self(), ident).NAME();                                           \
  }                                                                            \
  __pure_function RETURN_TYPE NAME(const token &ident) const;
  HERE_CRTP_MAKE(const tuple_ro *, get_nested)
  HERE_CRTP_MAKE(string_view, get_string)
  HERE_CRTP_MAKE(string_view, get_varbinary)
  HERE_CRTP_MAKE(property_pair, get_property)
  HERE_CRTP_MAKE(bool, get_bool)
  HERE_CRTP_MAKE(short, get_enum)
  HERE_CRTP_MAKE(int8_t, get_i8)
  HERE_CRTP_MAKE(uint8_t, get_u8)
  HERE_CRTP_MAKE(int16_t, get_i16)
  HERE_CRTP_MAKE(uint16_t, get_u16)
  HERE_CRTP_MAKE(int32_t, get_i32)
  HERE_CRTP_MAKE(uint32_t, get_u32)
  HERE_CRTP_MAKE(int64_t, get_i64)
  HERE_CRTP_MAKE(uint64_t, get_u64)
  HERE_CRTP_MAKE(float, get_f32)
  HERE_CRTP_MAKE(double, get_f64)
  HERE_CRTP_MAKE(decimal64, get_decimal)
  HERE_CRTP_MAKE(datetime_t, get_datetime)
  HERE_CRTP_MAKE(const uuid_t &, get_uuid)
  HERE_CRTP_MAKE(int128_t, get_int128)
  HERE_CRTP_MAKE(uint128_t, get_uint128)
  HERE_CRTP_MAKE(const binary96_t &, get_b96)
  HERE_CRTP_MAKE(const binary128_t &, get_b128)
  HERE_CRTP_MAKE(const binary160_t &, get_b160)
  HERE_CRTP_MAKE(const binary192_t &, get_b192)
  HERE_CRTP_MAKE(const binary224_t &, get_b224)
  HERE_CRTP_MAKE(const binary256_t &, get_b256)
  HERE_CRTP_MAKE(const binary320_t &, get_b320)
  HERE_CRTP_MAKE(const binary384_t &, get_b384)
  HERE_CRTP_MAKE(const binary512_t &, get_b512)
  HERE_CRTP_MAKE(const ip_address_t &, get_ip_address)
  HERE_CRTP_MAKE(mac_address_t, get_mac_address)
  HERE_CRTP_MAKE(const ip_net_t &, get_ip_net)
  HERE_CRTP_MAKE(int64_t, get_integer)
  HERE_CRTP_MAKE(uint64_t, get_unsigned)
  HERE_CRTP_MAKE(double, get_float)
#undef HERE_CRTP_MAKE
  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  __pure_function inline collection_ro<TOKEN>
  collection(const TOKEN &ident) const {
    return collect(self(), ident);
  }
  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  __pure_function inline accessor_ro<TOKEN>
  operator[](const TOKEN &ident) const {
    return at(self(), ident);
  }
  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  __pure_function inline bool is_present(const TOKEN &ident) const {
    return have(self(), ident);
  }
  __pure_function dynamic_collection_ro collection(const token &ident) const;
  __pure_function dynamic_accessor_ro operator[](const token &ident) const;
  __pure_function bool is_present(const token &ident) const;
};

} // namespace details
} // namespace fptu

#include "fast_positive/details/warnings_pop.h"
