/*
 *  Fast Positive Tuples (libfptu), aka Позитивные Кортежи
 *  Copyright 2016-2019 Leonid Yuriev <leo@yuriev.ru>
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

#ifndef __AVX__
#error "The -mavx or /arch:AVX compiler's option is required"
#endif

#include "fast_positive/erthink/erthink_intrin.h"
#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/details/field.h"
#include "fast_positive/tuples/details/scan.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "fast_positive/erthink/erthink_optimize4speed.h"

namespace fptu {
namespace details {

static __always_inline unsigned cmp2mask(const __m256 pattern,
                                         const field_loose *scan) {
  const __m256 odd = _mm256_castsi256_ps(_mm256_set1_epi32(0xffff0000u));
  return _mm256_movemask_ps(_mm256_cmp_ps(
      pattern,
      _mm256_and_ps(
          odd, _mm256_castsi256_ps(_mm256_loadu_si256((const __m256i *)scan))),
      _CMP_EQ_OQ));
}

#if __SANITIZE_ADDRESS__
static __m256i __attribute__((no_sanitize_address, noinline))
bypass_asan_read_m256(const void *src) {
  return _mm256_loadu_si256((const __m256i *)src);
}
#define cmp2mask_bypass_asan(pattern, scan)                                    \
  ({                                                                           \
    __m256i copy = bypass_asan_read_m256(scan);                                \
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
  ptr += index;
  return true;
}

#define STEP_x8                                                                \
  do {                                                                         \
    if (mask2ptr(cmp2mask(pattern, scan), scan))                               \
      return scan;                                                             \
    scan += 8;                                                                 \
  } while (0)

__hot const field_loose *fptu_scan_AVX(const field_loose *begin,
                                       const field_loose *end,
                                       uint16_t genius_and_id) {
  const ptrdiff_t bytes = (char *)end - (char *)begin;
  assert(bytes % 4 == 0);

  const __m256 pattern = _mm256_permute2f128_ps(
      _mm256_castps128_ps256(
          _mm_castsi128_ps(_mm_set1_epi32((uint32_t)genius_and_id << 16))),
      _mm256_undefined_ps(), 0);
  const field_loose *scan = begin;

  if (unlikely(bytes < 8 * 4)) {
    if (unlikely(bytes < 4))
      /* empty or hollow tuple case */
      return nullptr;

    unsigned mask;
    if (likely(/* check for enough on-page offset for '-32' */ 0xfe0 &
               (uintptr_t)scan)) {
      mask = cmp2mask_bypass_asan(pattern, end - 8);
      mask >>= (8 - bytes / 4);
    } else {
      mask = 255 >> (8 - bytes / 4);
      mask &= cmp2mask_bypass_asan(pattern, scan);
    }
    if (mask2ptr(mask, scan))
      return scan;
    return nullptr;
  }

  std::size_t chunks = (bytes + 31) >> 5;
  while (1)
    switch (chunks) {
    default:
      do {
        STEP_x8;
        STEP_x8;
        STEP_x8;
        STEP_x8;
        STEP_x8;
        STEP_x8;
        STEP_x8;
        STEP_x8;
      } while (scan < end - 63);
      assert((bytes & 255) == ((char *)end - (char *)scan));
      chunks = ((bytes & 255) + 31) >> 5;
      assert((ptrdiff_t)chunks == ((char *)end - (char *)scan + 31) / 32);
      continue;

    case 8:
      STEP_x8;
      __fallthrough /* fall through */;
    case 7:
      STEP_x8;
      __fallthrough /* fall through */;
    case 6:
      STEP_x8;
      __fallthrough /* fall through */;
    case 5:
      STEP_x8;
      __fallthrough /* fall through */;
    case 4:
      STEP_x8;
      __fallthrough /* fall through */;
    case 3:
      STEP_x8;
      __fallthrough /* fall through */;
    case 2:
      STEP_x8;
      __fallthrough /* fall through */;
    case 1:
      scan = end - 8;
      assert(begin <= scan);
      STEP_x8;
      __fallthrough /* fall through */;
    case 0:
      return nullptr;
    }
}

} // namespace details
} // namespace fptu
