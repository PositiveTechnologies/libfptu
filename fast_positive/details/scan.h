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

#pragma once

#include "fast_positive/details/api.h"
#include "fast_positive/details/erthink/erthink_arch.h"
#include "fast_positive/details/erthink/erthink_ifunc.h"
#include "fast_positive/details/field.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fptu {
namespace details {

typedef const field_loose *(*scan_func_t)(const field_loose *begin,
                                          const field_loose *end,
                                          uint16_t genius_and_id);

/* Artless reference implementation for testing & verification */
__extern_C FPTU_API const field_loose *
fptu_scan_referential(const field_loose *begin, const field_loose *end,
                      uint16_t genius_and_id);

__extern_C FPTU_API const field_loose *
fptu_scan_unroll(const field_loose *begin, const field_loose *end,
                 uint16_t genius_and_id);

#if defined(__ia32__) || defined(__e2k__)
__extern_C FPTU_API const field_loose *fptu_scan_AVX(const field_loose *begin,
                                                     const field_loose *end,
                                                     uint16_t genius_and_id);
#endif /* __ia32__ || __e2k__ */

#if defined(__ia32__)
__extern_C FPTU_API const field_loose *fptu_scan_SSE2(const field_loose *begin,
                                                      const field_loose *end,
                                                      uint16_t genius_and_id);
__extern_C FPTU_API const field_loose *fptu_scan_AVX2(const field_loose *begin,
                                                      const field_loose *end,
                                                      uint16_t genius_and_id);
#endif /* __ia32__ */

#if defined(__AVX2__)
#define fptu_scan(begin, end, genius_and_id)                                   \
  fptu_scan_AVX2(begin, end, genius_and_id)
#elif defined(__e2k__)
#define fptu_scan(begin, end, genius_and_id)                                   \
  fptu_scan_AVX(begin, end, genius_and_id)
#elif defined(__ATOM__)
#define fptu_scan(begin, end, genius_and_id)                                   \
  fptu_scan_SSE2(begin, end, genius_and_id)
#elif defined(__ia32__)
ERTHINK_DECLARE_IFUNC(FPTU_API, const field_loose *, fptu_scan,
                      (const field_loose *begin, const field_loose *end,
                       uint16_t genius_and_id),
                      (begin, end, genius_and_id), fptu_scan_resolver)
#else
#define fptu_scan(begin, end, genius_and_id)                                   \
  fptu_scan_unroll(begin, end, genius_and_id)
#endif

//------------------------------------------------------------------------------

static __pure_function inline const field_loose *
lookup(bool sorted, const field_loose *begin, const field_loose *end,
       const tag_t tag) noexcept {
  assert(is_loose(tag));
  static_assert(sizeof(field_loose) == 4 && sizeof(unit_t) == 4, "WTF?");
  (void)/* TODO: use search() for sorted tuples */ sorted;
  return fptu_scan(begin, end, uint16_t(tag));
}

static __pure_function inline const field_loose *
next(const field_loose *current, const field_loose *end,
     const tag_t tag) noexcept {
  assert(is_loose(tag));
  static_assert(sizeof(field_loose) == 4 && sizeof(unit_t) == 4, "WTF?");
  return current ? fptu_scan(current + 1, end, uint16_t(tag)) : current;
}

} // namespace details
} // namespace fptu
