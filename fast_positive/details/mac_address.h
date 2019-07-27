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

#include <stdint.h>

typedef union FPTU_API_TYPE fptu_mac_address {
  uint8_t u8[6];
  uint8_t u16[3];
  uint64_t raw64;
#ifdef __cplusplus
  fptu_mac_address() = default;
  explicit constexpr fptu_mac_address(uint64_t u) : raw64(u) {
    constexpr_assert(raw64 <= UINT64_C(0xffffFFFFffff));
  }
#endif /* __cplusplus */
} fptu_mac_address_t;

#ifdef __cplusplus
namespace fptu {
typedef ::fptu_mac_address_t mac_address_t;
} // namespace fptu
#endif /* __cplusplus */
