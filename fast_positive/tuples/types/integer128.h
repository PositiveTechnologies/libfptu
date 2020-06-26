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

#include <stdint.h>

#if defined(__SIZEOF_INT128__) ||                                              \
    (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 128)

typedef __uint128_t fptu_uint128_t;
typedef __int128_t fptu_int128_t;

#else

/* Non-operational stub 128-bit integer types, just for place holding */

struct FPTU_API_TYPE fptu_uint128 {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint64_t l, h;
#else
  uint64_t h, l;
#endif

#ifdef __cplusplus
  fptu_uint128() = default;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  cxx11_constexpr fptu_uint128(uint64_t u) : l(u), h(0) {}
#else
  cxx11_constexpr fptu_uint128(uint64_t u) : h(0), l(u) {}
#endif
  cxx11_constexpr operator uint64_t() const { return l; }
#endif /* __cplusplus */
};

struct FPTU_API_TYPE fptu_int128 {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  int64_t l, h;
#else
  int64_t h, l;
#endif

#ifdef __cplusplus
  fptu_int128() = default;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  cxx11_constexpr fptu_int128(int64_t i) : l(i), h(i >> 63) {}
  cxx11_constexpr fptu_int128(uint64_t i) : l(i), h(0) {}
#else
  cxx11_constexpr fptu_int128(int64_t i) : h(i >> 63), l(i) {}
  cxx11_constexpr fptu_int128(uint64_t i) : h(0), l(i) {}
#endif
  cxx11_constexpr operator int64_t() const { return l; }
#endif /* __cplusplus */
};

typedef struct fptu_uint128 fptu_uint128_t;
typedef struct fptu_int128 fptu_int128_t;

#endif /*  __SIZEOF_INT128__ || _INTEGRAL_MAX_BITS >= 128 */

#ifdef __cplusplus
namespace fptu {
typedef ::fptu_uint128_t uint128_t;
typedef ::fptu_int128_t int128_t;
} // namespace fptu
#endif /* __cplusplus */
