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
