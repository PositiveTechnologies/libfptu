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

#include "fast_positive/details/cpu_features.h"
#include "fast_positive/details/erthink/erthink_defs.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/scan.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fptu {
namespace details {

#ifndef fptu_scan

ERTHINK_DEFINE_IFUNC(FPTU_API, const field_loose *, fptu_scan,
                     (const field_loose *begin, const field_loose *end,
                      uint16_t genius_and_id),
                     (begin, end, genius_and_id), fptu_scan_resolver)

ERTHINK_IFUNC_RESOLVER_API(FPTU_API)
__cold scan_func_t fptu_scan_resolver() {
#ifdef __ia32__
  if (cpu_features.has_AVX2())
    return fptu_scan_AVX2;
  else if (cpu_features.has_AVX())
    return fptu_scan_AVX;
  else if (cpu_features.has_SSE2())
    return fptu_scan_SSE2;
#else
#warning "FIXME: Support for other architectures"
#endif
  return fptu_scan_unroll;
}

#endif /* fptu_scan */

} // namespace details
} // namespace fptu
