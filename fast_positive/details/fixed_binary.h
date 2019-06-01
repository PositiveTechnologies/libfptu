/*
 *  Fast Positive Tuples (libfptu)
 *  Copyright 2016-2019 Leonid Yuriev, https://github.com/leo-yuriev/libfptu
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

#pragma pack(push, 1)

typedef union FPTU_API_TYPE fptu_binary96 {
  uint8_t u8[96 / 8];
  uint16_t u16[96 / 16];
  uint32_t u32[96 / 32];
  uint64_t u64;
} fptu_binary96_t;

typedef union FPTU_API_TYPE fptu_binary128 {
  uint8_t u8[128 / 8];
  uint16_t u16[128 / 16];
  uint32_t u32[128 / 32];
  uint64_t u64[128 / 64];
  fptu_uint128_t u128;
  fptu_int128_t i128;
} fptu_binary128_t;

typedef union FPTU_API_TYPE fptu_binary160 {
  uint8_t u8[160 / 8];
  uint16_t u16[160 / 16];
  uint32_t u32[160 / 32];
} fptu_binary160_t;

typedef union FPTU_API_TYPE fptu_binary192 {
  uint8_t u8[192 / 8];
  uint16_t u16[192 / 16];
  uint32_t u32[192 / 32];
  uint64_t u64[192 / 64];
} fptu_binary192_t;

typedef union FPTU_API_TYPE fptu_binary224 {
  uint8_t u8[224 / 8];
  uint16_t u16[224 / 16];
  uint32_t u32[224 / 32];
} fptu_binary224_t;

typedef union FPTU_API_TYPE fptu_binary256 {
  uint8_t u8[256 / 8];
  uint16_t u16[256 / 16];
  uint32_t u32[256 / 32];
  uint64_t u64[256 / 64];
} fptu_binary256_t;

typedef union FPTU_API_TYPE fptu_binary320 {
  uint8_t u8[320 / 8];
  uint16_t u16[320 / 16];
  uint32_t u32[320 / 32];
  uint64_t u64[320 / 64];
} fptu_binary320_t;

typedef union FPTU_API_TYPE fptu_binary384 {
  uint8_t u8[384 / 8];
  uint16_t u16[384 / 16];
  uint32_t u32[384 / 32];
  uint64_t u64[384 / 64];
} fptu_binary384_t;

typedef union FPTU_API_TYPE fptu_binary512 {
  uint8_t u8[512 / 8];
  uint16_t u16[512 / 16];
  uint32_t u32[512 / 32];
  uint64_t u64[512 / 64];
} fptu_binary512_t;

#pragma pack(pop)

#ifdef __cplusplus
namespace fptu {

using binary96_t = fptu_binary96_t;
using binary128_t = fptu_binary128_t;
using binary160_t = fptu_binary160_t;
using binary192_t = fptu_binary192_t;
using binary224_t = fptu_binary224_t;
using binary256_t = fptu_binary256_t;
using binary320_t = fptu_binary320_t;
using binary384_t = fptu_binary384_t;
using binary512_t = fptu_binary512_t;

template <unsigned NBITS> struct fixed_binary_typemap {};
template <> struct fixed_binary_typemap<8> { using type = uint8_t; };
template <> struct fixed_binary_typemap<16> { using type = uint16_t; };
template <> struct fixed_binary_typemap<32> { using type = uint32_t; };
template <> struct fixed_binary_typemap<64> { using type = uint64_t; };
template <> struct fixed_binary_typemap<96> { using type = binary96_t; };
template <> struct fixed_binary_typemap<128> { using type = binary128_t; };
template <> struct fixed_binary_typemap<160> { using type = binary160_t; };
template <> struct fixed_binary_typemap<192> { using type = binary192_t; };
template <> struct fixed_binary_typemap<224> { using type = binary224_t; };
template <> struct fixed_binary_typemap<256> { using type = binary256_t; };
template <> struct fixed_binary_typemap<320> { using type = binary320_t; };
template <> struct fixed_binary_typemap<384> { using type = binary384_t; };
template <> struct fixed_binary_typemap<512> { using type = binary512_t; };

template <unsigned NBITS>
using fixed_binary = typename fixed_binary_typemap<NBITS>::type;

} // namespace fptu
#endif /* __cplusplus */
