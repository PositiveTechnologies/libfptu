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

#include "fast_positive/details/legacy_compat.h"

//------------------------------------------------------------------------------

bool fptu_field_is_dead(const fptu_field *pf) noexcept {
  return !pf || pf->is_hole();
}

fptu_type fptu_field_type(const fptu_field *pf) noexcept {
  return pf ? pf->legacy_type() : fptu_null;
}
int fptu_field_column(const fptu_field *pf) noexcept {
  return pf ? pf->colnum() : -1;
}

void fptu_erase_field(fptu_rw *pt, fptu_field *pf) noexcept {
  if (pf && !pf->is_hole())
    pt->erase(pf);
}

//------------------------------------------------------------------------------

struct iovec fptu_field_as_iovec(const fptu_field *pf) noexcept {
  if (likely(pf)) {
    const auto type = pf->type();
    if (fptu::details::is_inplaced(type))
      return fptu::iovec(&pf->inplaced, fptu::details::loose_units(type));

    if (fptu::details::is_fixed_size(type))
      return fptu::iovec(pf->relative.payload(),
                         fptu::details::loose_units(type));

    if (pf->relative.have_payload())
      return fptu::iovec(pf->relative.payload(),
                         pf->relative.payload()->stretchy.length(type));
  }
  return fptu::iovec(nullptr, 0);
}

#define FPTU_GET_IMPL(LEGACY, NAME, GENUS, RETURN_TYPE, THUNK_TYPE, DENIL)     \
  RETURN_TYPE fptu_field_##LEGACY(const fptu_field *pf) noexcept {             \
    error_guard raii(nullptr);                                                 \
    try {                                                                      \
      return THUNK_TYPE(fptu::details::get<fptu::genus::GENUS>(pf, false));    \
    } catch (const std::exception &e) {                                        \
      raii.feed(e);                                                            \
      return DENIL;                                                            \
    }                                                                          \
  }

FPTU_GET_IMPL(uint16, u16, u16, uint_fast16_t, uint_fast16_t, 0)
FPTU_GET_IMPL(bool, bool, boolean, bool, bool, false)
FPTU_GET_IMPL(int32, i32, i32, int_fast32_t, int_fast32_t, 0)
FPTU_GET_IMPL(uint32, u32, u32, uint_fast32_t, uint_fast32_t, 0)
FPTU_GET_IMPL(int64, i64, i64, int_fast64_t, int_fast64_t, 0)
FPTU_GET_IMPL(uint64, u64, u64, uint_fast64_t, uint_fast64_t, 0)
FPTU_GET_IMPL(fp64, f64, f64, double_t, double_t, FPTU_DENIL_FP64)
FPTU_GET_IMPL(fp32, f32, f32, float_t, float_t, FPTU_DENIL_FP32)
FPTU_GET_IMPL(datetime, datetime, t64, fptu_datetime_C, fptu_datetime_C,
              FPTU_DENIL_DATETIME)

FPTU_GET_IMPL(opaque, varbinary, varbin, ::iovec, fptu::details::iovec_thunk,
              fptu::details::iovec_thunk())
FPTU_GET_IMPL(nested, nested, nested, fptu_ro, fptu::details::iovec_thunk,
              fptu::details::iovec_thunk())
#undef FPTU_GET_IMPL

#define FPTU_GET_IMPL(BITS)                                                    \
  const uint8_t *fptu_field_##BITS(const fptu_field *pf) noexcept {            \
    error_guard raii(nullptr);                                                 \
    try {                                                                      \
      return erthink::constexpr_pointer_cast<const uint8_t *>(                 \
          &fptu::details::get<fptu::genus::b##BITS>(pf, false));               \
    } catch (const std::exception &e) {                                        \
      raii.feed(e);                                                            \
      return nullptr;                                                          \
    }                                                                          \
  }

FPTU_GET_IMPL(96)
FPTU_GET_IMPL(128)
FPTU_GET_IMPL(160)
FPTU_GET_IMPL(256)
#undef FPTU_GET_IMPL

/* LY: crutch for returning legacy c-string */
static const char *string_view_to_CAPI(const fptu::string_view &value,
                                       const char *detend) noexcept {
  static thread_local std::string holder;
  if (value.empty())
    return FPTU_DENIL_CSTR;

  const char *const cstr_begin = value.data();
  const char *const cstr_end = value.end();
  if (cstr_end < detend) {
    assert(*cstr_end == '\0');
    return cstr_begin;
  }

  holder.assign(cstr_begin, cstr_end);
  assert(holder.c_str()[holder.length()] == '\0');
  return holder.c_str();
}

const char *fptu_field_cstr(const fptu_field *pf) noexcept {
  error_guard raii(nullptr);
  if (likely(pf)) {
    try {
      const auto payload = pf->relative.payload();
      const char *const field_end =
          erthink::constexpr_pointer_cast<const char *>(
              payload->flat + payload->stretchy.string.brutto_units());
      return string_view_to_CAPI(
          fptu::details::get<fptu::genus::text>(pf, false), field_end);
    } catch (const std::exception &e) {
      raii.feed(e);
    }
  }
  return FPTU_DENIL_CSTR;
}
