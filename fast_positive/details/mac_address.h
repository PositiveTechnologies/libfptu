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

typedef union fptu_mac_address {
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
