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

#pragma once
#include "fast_positive/details/api.h"

#include "fast_positive/details/integer128.h"
#include <stdint.h>

//------------------------------------------------------------------------------

#ifdef _GLIBCXX_USE_FLOAT128

typedef __float128 fptu_float128_t;

#else

/* Non-operational stub 128-bit float point, just for place holding */

typedef union FPTU_API_TYPE fptu_float128 {
  fptu_uint128_t u128;
  fptu_int128_t i128;
} fptu_float128_t;

#endif /* _GLIBCXX_USE_FLOAT128 */

//------------------------------------------------------------------------------

/* 16-bits IEEE 754-2008
 * https://en.wikipedia.org/wiki/Half-precision_floating-point_format
 */

/* Non-operational stub 16-bit float point, just for place holding */
typedef union FPTU_API_TYPE fptu_float16 {
  int16_t i16;
  uint16_t u16;
} fptu_float16_t;

//------------------------------------------------------------------------------

#ifdef __cplusplus
namespace fptu {
typedef ::fptu_float128_t float128_t;
typedef ::fptu_float16_t float16_t;
} // namespace fptu
#endif /* __cplusplus */
