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

#include "fast_positive/details/api.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/scan.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fptu {
namespace details {

//------------------------------------------------------------------------------

/* Artless reference implementation for testing & verification */
__cold const field_loose *fptu_scan_referential(const field_loose *begin,
                                                const field_loose *end,
                                                uint16_t genius_and_id) {
  for (const field_loose *scan = begin; scan < end; ++scan)
    if (genius_and_id == scan->genius_and_id)
      return scan;
  return nullptr;
}

//------------------------------------------------------------------------------

#include "fast_positive/details/erthink/erthink_optimize4speed.h"

#define STEP_x1                                                                \
  do {                                                                         \
    if (genius_and_id == scan->genius_and_id)                                  \
      return scan;                                                             \
    ++scan;                                                                    \
  } while (0)

__hot const field_loose *fptu_scan_unroll(const field_loose *begin,
                                          const field_loose *end,
                                          uint16_t genius_and_id) {
  ptrdiff_t items = end - begin;

  const field_loose *scan = begin;
  while (1)
    switch (size_t(items)) {
    default:
      if (unlikely(items < 0))
        /* hollow tuple case */
        return nullptr;
      do {
        STEP_x1;
        STEP_x1;
        STEP_x1;
        STEP_x1;
        STEP_x1;
        STEP_x1;
        STEP_x1;
        STEP_x1;
      } while (scan < end - 7);
      assert((items & 7) * 4 == (char *)end - (char *)scan);
      items &= 7;
      continue;
    case 8:
      STEP_x1;
      __fallthrough /* fall through */;
    case 7:
      STEP_x1;
      __fallthrough /* fall through */;
    case 6:
      STEP_x1;
      __fallthrough /* fall through */;
    case 5:
      STEP_x1;
      __fallthrough /* fall through */;
    case 4:
      STEP_x1;
      __fallthrough /* fall through */;
    case 3:
      STEP_x1;
      __fallthrough /* fall through */;
    case 2:
      STEP_x1;
      __fallthrough /* fall through */;
    case 1:
      STEP_x1;
      __fallthrough /* fall through */;
    case 0:
      return nullptr;
    }
}

} // namespace details
} // namespace fptu
