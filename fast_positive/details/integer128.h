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

#pragma once
#include "fast_positive/details/api.h"

#include <stdint.h>

#if defined(__SIZEOF_INT128__) ||                                              \
    (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 128)

typedef __uint128_t fptu_uint128_t;
typedef __int128_t fptu_int128_t;

#else

/* Non-operational stub 128-bit integer types, just for place holding */

struct fptu_uint128 {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint64_t l, h;
#else
  uint64_t h, l;
#endif

#ifdef __cplusplus
  fptu_uint128() = default;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  constexpr fptu_uint128(uint64_t u) : l(u), h(0) {}
#else
  constexpr fptu_uint128(uint64_t u) : h(0), l(u) {}
#endif
  constexpr operator uint64_t() const { return l; }
#endif /* __cplusplus */
};

struct fptu_int128 {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  int64_t l, h;
#else
  int64_t h, l;
#endif

#ifdef __cplusplus
  fptu_int128() = default;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  constexpr fptu_int128(int64_t i) : l(i), h(i >> 63) {}
  constexpr fptu_int128(uint64_t i) : l(i), h(0) {}
#else
  constexpr fptu_int128(int64_t i) : h(i >> 63), l(i) {}
  constexpr fptu_int128(uint64_t i) : h(0), l(i) {}
#endif
  constexpr operator int64_t() const { return l; }
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
