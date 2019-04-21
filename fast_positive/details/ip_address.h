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

#include "fast_positive/details/erthink/erthink_bswap.h"
#include "fast_positive/details/erthink/erthink_byteorder.h"
#include "fast_positive/details/fixed_binary.h"
#include <stdint.h>

#if defined(_MSC_VER) && !defined(htobe32)
/* TODO: move to erthink_byteorder.h */
#define htobe16(le16) _byteswap_ushort(le16)
#define be16toh(be16) _byteswap_ushort(be16)
#define htobe32(le32) _byteswap_ulong(le32)
#define be32toh(be32) _byteswap_ulong(be32)
#define htobe64(le64) _byteswap_uint64(le64)
#define be64toh(be64) _byteswap_uint64(be64)
#endif

typedef union fptu_ip_address {
  uint8_t u8[16];
  uint16_t u16[8];
  uint32_t u32[4];
  uint64_t u64[2];
#ifdef __cplusplus
  fptu_ip_address() = default;
  explicit constexpr fptu_ip_address(uint32_t ipv4_be)
      : u32{0, 0, ipv4_be ? htobe32(0xffffu) : 0u, ipv4_be} {}
#endif /* __cplusplus */
} fptu_ip_address_t;

#pragma pack(push, 1)
typedef struct fptu_ip_net {
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
