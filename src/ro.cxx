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

/*
 * libfptu = { Fast Positive Tuples, aka Позитивные Кортежи }
 *
 * The kind of lightweight linearized tuples, which are extremely handy
 * to machining, including cases with shared memory.
 * Please see README.md at https://github.com/leo-yuriev/libfptu
 *
 * The Future will Positive. Всё будет хорошо.
 *
 * "Позитивные Кортежи" дают легковесное линейное представление небольших
 * JSON-подобных структур в экстремально удобной для машины форме,
 * в том числе при размещении в разделяемой памяти.
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

__hot const char *tuple_ro::audit(const void *ptr, std::size_t bytes,
                                  const schema *schema,
                                  audit_holes_info &holes_info) {

  if (unlikely(ptr == nullptr))
    return "hollow tuple (nullptr)";

  if (unlikely(bytes < sizeof(unit_t)))
    return "hollow tuple (too short)";
  if (unlikely(bytes > fptu::max_tuple_bytes))
    return "tuple too large";
  if (unlikely(bytes % sizeof(unit_t)))
    return "odd tuple size";

  const tuple_ro *const self = static_cast<const tuple_ro *>(ptr);
  if (unlikely(bytes != self->size()))
    return "tuple size mismatch";

  const std::size_t index_size = self->index_size();
  if (unlikely(index_size > fptu::max_fields))
    return "index too large (many loose fields)";

  if (self->pivot() > self->end_data_bytes())
    return "tuple.pivot > tuple.end";

  audit_flags flags = audit_flags::audit_default;
  if (self->is_sorted())
    flags |= audit_flags::audit_tuple_sorted_loose;
  if (self->have_preplaced())
    flags |= audit_flags::audit_tuple_have_preplaced;

  return audit_tuple(schema, self->begin_index(), self->pivot(),
                     self->end_data_bytes(), flags, holes_info);
}

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

// template class FPTU_API accessor_ro<token>;
// template class FPTU_API iterator_ro<token>;
// template class FPTU_API collection_ro<token>;

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

} // namespace fptu
