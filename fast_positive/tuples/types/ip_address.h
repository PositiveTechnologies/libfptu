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

#include "fast_positive/erthink/erthink_endian.h"
#include "fast_positive/tuples/types/fixed_binary.h"
#include <stdint.h>

typedef union FPTU_API_TYPE fptu_ip_address {
  uint8_t u8[16];
  uint16_t u16[8];
  uint32_t u32[4];
  uint64_t u64[2];
#ifdef __cplusplus
  fptu_ip_address() = default;
  explicit constexpr fptu_ip_address(uint32_t ipv4_be)
      : u32{0, 0, ipv4_be ? erthink::h2be<uint32_t>(0xffffu) : 0u, ipv4_be} {}
#endif /* __cplusplus */
} fptu_ip_address_t;

#pragma pack(push, 1)
typedef struct FPTU_API_TYPE fptu_ip_net {
  fptu_ip_address_t address;
  uint8_t cidr;
#ifdef __cplusplus
  fptu_ip_net() = default;
  explicit constexpr fptu_ip_net(uint32_t ipv4_be)
      : address(ipv4_be), cidr(ipv4_be ? 128 : 0) {}
#endif /* __cplusplus */
} fptu_ip_net_t;
#pragma pack(pop)

#ifdef __cplusplus
namespace fptu {
typedef ::fptu_ip_address_t ip_address_t;
typedef ::fptu_ip_net_t ip_net_t;
} // namespace fptu
#endif /* __cplusplus */
