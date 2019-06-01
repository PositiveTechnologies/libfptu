/*
 *  Fast Positive Tuples (libfptu)
 *  Copyright 2016-2019 Leonid Yuriev, https://github.com/leo-yuriev/libfptu
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

#include "fast_positive/details/ro.h"
#include "fast_positive/details/api.h"
#include "fast_positive/details/audit.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/exceptions.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/getter.h"
#include "fast_positive/details/meta.h"
#include "fast_positive/details/scan.h"
#include "fast_positive/details/schema.h"
#include "fast_positive/details/token.h"

#include "fast_positive/tuples_internal.h"
#include <algorithm>

#include "fast_positive/details/erthink/erthink_optimize4speed.h"

namespace fptu {
namespace details {

const tuple_ro *tuple_ro::make_from_buffer(const void *ptr, std::size_t bytes,
                                           const schema *schema,
                                           bool skip_validation) {
  if (unlikely(!ptr || bytes < sizeof(stretchy_value_tuple))) {
    if (skip_validation)
      return nullptr;
    throw_tuple_hollow();
  }

  if (!skip_validation) {
    const char *trouble = audit(ptr, bytes, schema, false);
    if (unlikely(trouble))
      throw_tuple_bad(schema, ptr, bytes, trouble);
  }

  return static_cast<const tuple_ro *>(ptr);
}

//------------------------------------------------------------------------------

#define HERE_CRTP_MAKE(RETURN_TYPE, NAME)                                      \
  template <>                                                                  \
  FPTU_API __pure_function RETURN_TYPE crtp_getter<tuple_ro>::NAME(            \
      const token &ident) const {                                              \
    return at(self(), ident).NAME();                                           \
  }
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

template <>
FPTU_API __pure_function dynamic_collection_ro
crtp_getter<tuple_ro>::collection(const token &ident) const {
  return collect(self(), ident);
}

template <>
FPTU_API __pure_function dynamic_accessor_ro crtp_getter<tuple_ro>::
operator[](const token &ident) const {
  return at(self(), ident);
}

template <>
FPTU_API __pure_function bool
crtp_getter<tuple_ro>::is_present(const token &ident) const {
  return have(self(), ident);
}

// template class FPTU_API_TYPE accessor_ro<token>;
// template class FPTU_API_TYPE iterator_ro<token>;
// template class FPTU_API_TYPE collection_ro<token>;

} // namespace details

// template class tuple_crtp_reader<tuple_ro_weak>;

__cold __noreturn void tuple_ro_managed::throw_buffer_mismatch() const {
  if (!get_buffer())
    throw managed_buffer_required(
        "tuple_ro_managed: managed 1Hippeus's buffer required for instance");
  else
    throw logic_error(
        get_impl()
            ? "tuple_ro_managed: tuple bounds are out of the managed buffer"
            : "tuple_ro_managed: tuple is nullptr, but buffer is provided");
}

//------------------------------------------------------------------------------

string_view /* preplaced_string::value_type */
preplaced_string::value() const {
  if (unlikely(nil()))
    throw_field_absent();

  return traits::read(payload());
}

string_view /* preplaced_varbin::value_type */
preplaced_varbin::value() const {
  if (unlikely(nil()))
    throw_field_absent();

  return traits::read(payload());
}

tuple_ro_weak /* preplaced_nested::value_type */
preplaced_nested::value() const {
  if (unlikely(nil()))
    throw_field_absent();

  return tuple_ro_weak(
      erthink::constexpr_pointer_cast<const details::tuple_ro *>(
          traits::read(payload())));
}

property_pair /* preplaced_property::value_type */
preplaced_property::value() const {
  if (unlikely(nil()))
    throw_field_absent();

  return traits::read(payload());
}

} // namespace fptu
