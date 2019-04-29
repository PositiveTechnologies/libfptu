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

#include "fast_positive/details/integer128.h"
#include <stdint.h>

#if defined(_GLIBCXX_USE_DECIMAL_FLOAT) && defined(__DECIMAL_BID_FORMAT__)

#include <decimal/decimal>
namespace fptu {
using std::decimal::decimal128;
using std::decimal::decimal32;
using std::decimal::decimal64;
} // namespace fptu

#else

/* Non-operational stub decimal float point types, just for place holding */

typedef union FPTU_API_TYPE fptu_decimal32 {
  uint32_t u32;
  int32_t i32;
#ifdef __cplusplus
  fptu_decimal32() = default;
  explicit constexpr fptu_decimal32(short i) : i32(i) {
    constexpr_assert(i == 0);
  }
#endif /* __cplusplus */
} fptu_decimal32_t;

typedef union FPTU_API_TYPE fptu_decimal64 {
  uint64_t u64;
  int64_t i64;
#ifdef __cplusplus
  fptu_decimal64() = default;
  explicit constexpr fptu_decimal64(short i) : i64(i) {
    constexpr_assert(i == 0);
  }
#endif /* __cplusplus */
} fptu_decimal64_t;

typedef union FPTU_API_TYPE fptu_decimal128 {
  fptu_int128_t u128;
  fptu_int128_t i128;
#ifdef __cplusplus
  fptu_decimal128() = default;
  explicit constexpr fptu_decimal128(int64_t i) : i128(i) {}
  explicit constexpr fptu_decimal128(uint64_t u) : u128(u) {}
#endif /* __cplusplus */
} fptu_decimal128_t;

#ifdef __cplusplus
namespace fptu {
typedef ::fptu_decimal32_t decimal32;
typedef ::fptu_decimal64_t decimal64;
typedef ::fptu_decimal128_t decimal128;
} // namespace fptu
#endif /* __cplusplus */

#endif /* _GLIBCXX_USE_DECIMAL_FLOAT && __DECIMAL_BID_FORMAT__ */
