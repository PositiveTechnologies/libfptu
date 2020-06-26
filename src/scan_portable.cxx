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

#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/details/field.h"
#include "fast_positive/tuples/details/scan.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fptu {
namespace details {

//------------------------------------------------------------------------------

/* Artless reference implementation for testing & verification */
__cold const field_loose *fptu_scan_referential(const field_loose *begin,
                                                const field_loose *end,
                                                uint16_t genus_and_id) {
  for (const field_loose *scan = begin; scan < end; ++scan)
    if (genus_and_id == scan->genus_and_id)
      return scan;
  return nullptr;
}

//------------------------------------------------------------------------------

#include "fast_positive/erthink/erthink_optimize4speed.h"

#define STEP_x1                                                                \
  do {                                                                         \
    if (genus_and_id == scan->genus_and_id)                                    \
      return scan;                                                             \
    ++scan;                                                                    \
  } while (0)

__hot const field_loose *fptu_scan_unroll(const field_loose *begin,
                                          const field_loose *end,
                                          uint16_t genus_and_id) {
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
