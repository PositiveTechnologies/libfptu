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
#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/details/exceptions.h"
#include "fast_positive/tuples/details/field.h"
#include "fast_positive/tuples/details/legacy_common.h"
#include "fast_positive/tuples/details/meta.h"
#include "fast_positive/tuples/details/scan.h"
#include "fast_positive/tuples/essentials.h"
#include "fast_positive/tuples/schema.h"
#include "fast_positive/tuples/token.h"

#include <iterator>

#include "fast_positive/tuples/details/warnings_push_pt.h"

namespace fptu {
namespace details {

template <typename> class crtp_getter;
template <typename> class accessor_ro;
template <typename> class collection_iterator_ro;
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
get(const field_loose *loose, const bool is_discernible_null) {
  if (likely(loose)) {
    if (unlikely(loose->type() != GENUS))
      throw_type_mismatch();
    return meta::genus_traits<GENUS>::read(loose);
  }
  if (unlikely(is_discernible_null))
    throw_field_absent();
  return meta::genus_traits<GENUS>::empty();
}

template <genus GENUS, typename erthink::enable_if_t<
                           meta::genus_traits<GENUS>::physique ==
                           meta::physique_kind::stretchy> * = nullptr>
inline cxx14_constexpr typename meta::genus_traits<GENUS>::return_type
get(const field_loose *loose, const bool is_discernible_null) {
  if (likely(loose)) {
    if (unlikely(loose->type() != GENUS))
      throw_type_mismatch();
    if (likely(loose->relative.have_payload()))
      return meta::genus_traits<GENUS>::read(loose->relative.payload());
  }
  if (unlikely(is_discernible_null))
    throw_field_absent();
  return meta::genus_traits<GENUS>::empty();
}

template <typename TOKEN> class accessor_ro {
protected:
  const void *field_;
  TOKEN token_;
  /* for a non-static tokens here is the 32-bit padding on 64-bit platforms due
   * alignment */

  template <typename> friend class collection_iterator_ro;
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
      if (token_.is_discernible_null() &&
          unlikely(meta::genus_traits<GENUS>::is_denil(
              static_cast<const field_preplaced *>(field_))))
        throw_field_absent();
      return meta::genus_traits<GENUS>::read(
          static_cast<const field_preplaced *>(field_));
    }

    if (likely(field_)) {
      // loose exist
      assert(
          tag2genus(static_cast<const field_loose *>(field_)->genus_and_id) ==
          GENUS);
      return meta::genus_traits<GENUS>::read(
          static_cast<const field_loose *>(field_));
    }
    //  loose absence
    if (unlikely(token_.is_discernible_null()))
      throw_field_absent();
    return meta::genus_traits<GENUS>::empty();
  }

  template <genus GENUS, typename erthink::enable_if_t<
                             meta::genus_traits<GENUS>::physique ==
                             meta::physique_kind::stretchy> * = nullptr>
  inline cxx14_constexpr typename meta::genus_traits<GENUS>::return_type
  get() const {
    if (unlikely(token_.type() != GENUS))
      throw_type_mismatch();

    if (likely(token_.is_preplaced() || field_)) {
      assert(
          token_.is_preplaced() ||
          tag2genus(static_cast<const field_loose *>(field_)->genus_and_id) ==
              GENUS);
      static_assert(offsetof(field_preplaced, relative) ==
                        offsetof(field_loose, relative),
                    "WTF?");
      const auto &relative =
          static_cast<const field_preplaced *>(field_)->relative;
      if (likely(relative.have_payload()))
        return meta::genus_traits<GENUS>::read(relative.payload());
    }

    // loose field or value is absent
    if (unlikely(token_.is_discernible_null()))
      throw_field_absent();
    return meta::genus_traits<GENUS>::empty();
  }

  // default constructor for empty/invalid value
  constexpr accessor_ro() : field_((void *)(nullptr)), token_() {
    static_assert(sizeof(*this) <= (TOKEN::is_static_token::value
                                        ? sizeof(void *)
                                        : sizeof(void *) * 2),
                  "WTF?");
  }

  // for preplaced fields
  constexpr accessor_ro(const field_preplaced *target,
                        const TOKEN token) noexcept
      : field_(target), token_(token) {}

  // for loose fields
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
  constexpr const accessor_ro *operator->() const noexcept { return this; }

  constexpr bool exist() const noexcept {
    return token_.is_preplaced()
               ? static_cast<const field_preplaced *>(field_)->is_null(
                     token_.tag())
               : field_ != nullptr;
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
  constexpr datetime_t get_datetime() const {
    return unlikely(type() == genus::t32) ? datetime_t::from_seconds(get<t32>())
                                          : get<t64>();
  }
  constexpr const uuid_t &get_uuid() const {
    return *erthink::constexpr_pointer_cast<const uuid_t *>(&get<bin128>());
  }

  constexpr const binary96_t &get_bin96() const {
    return *erthink::constexpr_pointer_cast<const binary96_t *>(&get<bin96>());
  }
  constexpr const binary128_t &get_bin128() const {
    return *erthink::constexpr_pointer_cast<const binary128_t *>(
        &get<bin128>());
  }
  constexpr const binary160_t &get_bin160() const {
    return *erthink::constexpr_pointer_cast<const binary160_t *>(
        &get<bin160>());
  }
  constexpr const binary192_t &get_bin192() const {
    return *erthink::constexpr_pointer_cast<const binary192_t *>(
        &get<bin192>());
  }
  constexpr const binary224_t &get_bin224() const {
    return *erthink::constexpr_pointer_cast<const binary224_t *>(
        &get<bin224>());
  }
  constexpr const binary256_t &get_bin256() const {
    return *erthink::constexpr_pointer_cast<const binary256_t *>(
        &get<bin256>());
  }
  constexpr const binary320_t &get_bin320() const {
    return *erthink::constexpr_pointer_cast<const binary320_t *>(
        &get<bin320>());
  }
  constexpr const binary384_t &get_bin384() const {
    return *erthink::constexpr_pointer_cast<const binary384_t *>(
        &get<bin384>());
  }
  constexpr const binary512_t &get_bin512() const {
    return *erthink::constexpr_pointer_cast<const binary512_t *>(
        &get<bin512>());
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
    if (type() == bin128)
      return *erthink::constexpr_pointer_cast<const int128_t *>(&get<bin128>());
    else
      return int128_t(get_integer());
  }
  cxx14_constexpr uint128_t get_uint128() const {
    if (type() == bin128)
      return *erthink::constexpr_pointer_cast<const uint128_t *>(
          &get<bin128>());
    else
      return uint128_t(get_unsigned());
  }
};

template <typename TOKEN>
class collection_iterator_ro
    : protected accessor_ro<TOKEN>
#if __cplusplus < 201703L
    ,
      public std::iterator<std::forward_iterator_tag, const accessor_ro<TOKEN>,
                           std::ptrdiff_t, const accessor_ro<TOKEN> *,
                           const accessor_ro<TOKEN> &>
#endif
{
  const field_loose *detent_ /* Указывает на конец индекса loose-дескприторов на
                                момент создания итератора. Требуется для
                                своевременной остановки поиска следующего
                                подходящего элемента в индексе
                                loose-дескрипторов. */
      ;

protected:
  template <typename> friend class collection_ro;
  friend class tuple_rw;
  using accessor = accessor_ro<TOKEN>;

  explicit constexpr collection_iterator_ro(const field_loose *target,
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

  constexpr collection_iterator_ro() noexcept
      : detent_(static_cast<const field_loose *>(accessor::field_)) {
    static_assert(sizeof(*this) <= (TOKEN::is_static_token::value
                                        ? sizeof(void *) * 2
                                        : sizeof(void *) * 3),
                  "WTF?");
  }
  constexpr collection_iterator_ro(const collection_iterator_ro &) noexcept =
      default;
  cxx14_constexpr collection_iterator_ro &
  operator=(const collection_iterator_ro &) noexcept = default;
  constexpr const TOKEN &token() const noexcept { return accessor::token(); }

  collection_iterator_ro &operator++() noexcept {
    assert(static_cast<const field_loose *>(accessor::field_) < detent_);
    accessor::field_ = next(static_cast<const field_loose *>(accessor::field_),
                            detent_, accessor::token_.tag());
    return *this;
  }
  collection_iterator_ro operator++(int) const noexcept {
    collection_iterator_ro iterator(*this);
    ++iterator;
    return iterator;
  }
  constexpr const accessor &operator*() const noexcept { return *this; }

  constexpr bool operator==(const accessor &other) const noexcept {
    return accessor::field_ == other.field_;
  }
  constexpr bool operator!=(const accessor &other) const noexcept {
    return accessor::field_ != other.field_;
  }
  constexpr bool operator==(const collection_iterator_ro &other) const
      noexcept {
    return accessor::field_ == other.field_;
  }
  constexpr bool operator!=(const collection_iterator_ro &other) const
      noexcept {
    return accessor::field_ != other.field_;
  }
};

template <typename TOKEN> class collection_ro {
  template <typename> friend class crtp_getter;
  friend class tuple_rw;

  collection_iterator_ro<TOKEN> iterator_;

  constexpr collection_ro(const field_loose *first, const field_loose *detent,
                          const TOKEN token) noexcept
      : iterator_(first, detent, token) {
    constexpr_assert(token.is_collection());
  }

public:
  using const_iterator = collection_iterator_ro<TOKEN>;
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
using dynamic_collection_iterator_ro = details::collection_iterator_ro<token>;
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
  HERE_CRTP_MAKE(const binary96_t &, get_bin96)
  HERE_CRTP_MAKE(const binary128_t &, get_bin128)
  HERE_CRTP_MAKE(const binary160_t &, get_bin160)
  HERE_CRTP_MAKE(const binary192_t &, get_bin192)
  HERE_CRTP_MAKE(const binary224_t &, get_bin224)
  HERE_CRTP_MAKE(const binary256_t &, get_bin256)
  HERE_CRTP_MAKE(const binary320_t &, get_bin320)
  HERE_CRTP_MAKE(const binary384_t &, get_bin384)
  HERE_CRTP_MAKE(const binary512_t &, get_bin512)
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

#include "fast_positive/tuples/details/warnings_pop.h"
