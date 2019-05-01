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

#if !defined(__SSE2__) && ((defined(_M_IX86_FP) && _M_IX86_FP >= 2) ||         \
                           defined(_M_AMD64) || defined(_M_X64))
#define __SSE2__
#endif

#ifndef __SSE2__
#error "The -msse2 or /arch:SSE2 compiler's option is required"
#endif

#include "fast_positive/details/api.h"
#include "fast_positive/details/erthink/erthink_intrin.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/scan.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "fast_positive/details/erthink/erthink_optimize4speed.h"

namespace fptu {
namespace details {

static __always_inline unsigned cmp2mask(const __m128i pattern,
                                         const field_loose *scan) {
  return _mm_movemask_epi8(
      _mm_cmpeq_epi16(pattern, _mm_loadu_si128((const __m128i *)scan)));
}

#if __SANITIZE_ADDRESS__
static __m128i __attribute__((no_sanitize_address, noinline))
bypass_asan_read_m128(const void *src) {
  return _mm_loadu_si128((const __m128i *)src);
}
#define cmp2mask_bypass_asan(pattern, scan)                                    \
  ({                                                                           \
    __m128i copy = bypass_asan_read_m128(scan);                                \
    cmp2mask(pattern, (field_loose *)&copy);                                   \
  })
#else
#define cmp2mask_bypass_asan(pattern, scan) cmp2mask(pattern, scan)
#endif

static __always_inline bool mask2ptr(unsigned mask, const field_loose *&ptr) {
  unsigned long index;
#ifdef _MSC_VER
  if (!_BitScanForward(&index, mask))
    return false;
#else
  if (!mask)
    return false;
  index = __builtin_ctz(mask);
#endif
  assert((index - 2) % 4 == 0);
  ptr = (const field_loose *)((char *)ptr + index - 2);
  return true;
}

#define STEP_x4                                                                \
  do {                                                                         \
    unsigned mask = cmp2mask(pattern, scan);                                   \
    if (mask2ptr(0x4444u & mask, scan))                                        \
      return scan;                                                             \
    scan += 4;                                                                 \
  } while (0)

__hot const field_loose *fptu_scan_SSE2(const field_loose *begin,
                                        const field_loose *end,
                                        uint16_t genius_and_id) {
  const ptrdiff_t bytes = (char *)end - (char *)begin;
  assert(bytes % 4 == 0);

  const __m128i pattern = _mm_set1_epi16(genius_and_id);
  const field_loose *scan = begin;

  if (unlikely(bytes < 4 * 4)) {
    if (unlikely(bytes < 4))
      /* empty or hollow tuple case */
      return nullptr;

    unsigned mask = 0x4444u;
    if (likely(/* check for enough on-page offset for '-16' */ 0xff0 &
               (uintptr_t)scan)) {
      mask &= cmp2mask_bypass_asan(pattern, end - 4);
      mask >>= (16 - bytes);
    } else {
      mask >>= (16 - bytes);
      mask &= cmp2mask_bypass_asan(pattern, scan);
    }
    if (mask2ptr(mask, scan))
      return scan;
    return nullptr;
  }

  std::size_t chunks = (bytes + 15) >> 4;
  while (1)
    switch (chunks) {
    default:
      do {
        STEP_x4;
        STEP_x4;
        STEP_x4;
        STEP_x4;
        STEP_x4;
        STEP_x4;
        STEP_x4;
        STEP_x4;
      } while (scan < end - 31);
      assert((bytes & 127) == ((char *)end - (char *)scan));
      chunks = ((bytes & 127) + 15) >> 4;
      assert((ptrdiff_t)chunks == ((char *)end - (char *)scan + 15) / 16);
      continue;

    case 8:
      STEP_x4;
      __fallthrough /* fall through */;
    case 7:
      STEP_x4;
      __fallthrough /* fall through */;
    case 6:
      STEP_x4;
      __fallthrough /* fall through */;
    case 5:
      STEP_x4;
      __fallthrough /* fall through */;
    case 4:
      STEP_x4;
      __fallthrough /* fall through */;
    case 3:
      STEP_x4;
      __fallthrough /* fall through */;
    case 2:
      STEP_x4;
      __fallthrough /* fall through */;
    case 1:
      scan = end - 4;
      assert(begin <= scan);
      STEP_x4;
      __fallthrough /* fall through */;
    case 0:
      return nullptr;
    }
}

} // namespace details
} // namespace fptu
