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

#include "fast_positive/tuples/types/integer128.h"
#include <stdint.h>

#if defined(_GLIBCXX_USE_DECIMAL_FLOAT) && defined(__DECIMAL_BID_FORMAT__) &&  \
    !defined(__COVERITY__)

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
  explicit cxx11_constexpr fptu_decimal32(short i) : i32(i) {
    constexpr_assert(i == 0);
  }
  friend bool operator==(const fptu_decimal32 a,
                         const fptu_decimal32 b) cxx11_noexcept {
    return a.u32 == b.u32;
  }
  friend bool operator!=(const fptu_decimal32 a,
                         const fptu_decimal32 b) cxx11_noexcept {
    return a.u32 != b.u32;
  }
#endif /* __cplusplus */
} fptu_decimal32_t;

typedef union FPTU_API_TYPE fptu_decimal64 {
  uint64_t u64;
  int64_t i64;
#ifdef __cplusplus
  fptu_decimal64() = default;
  explicit cxx11_constexpr fptu_decimal64(short i) : i64(i) {
    constexpr_assert(i == 0);
  }
  friend bool operator==(const fptu_decimal64 a,
                         const fptu_decimal64 b) cxx11_noexcept {
    return a.u64 == b.u64;
  }
  friend bool operator!=(const fptu_decimal64 a,
                         const fptu_decimal64 b) cxx11_noexcept {
    return a.u64 != b.u64;
  }
#endif /* __cplusplus */
} fptu_decimal64_t;

typedef union FPTU_API_TYPE fptu_decimal128 {
  fptu_int128_t u128;
  fptu_int128_t i128;
#ifdef __cplusplus
  fptu_decimal128() = default;
  explicit cxx11_constexpr fptu_decimal128(int64_t i) : i128(i) {}
  explicit cxx11_constexpr fptu_decimal128(uint64_t u) : u128(u) {}
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
