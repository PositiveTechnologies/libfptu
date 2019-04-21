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

#include "erthink/erthink_byteorder.h"
#include <cstdint>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error fixme..
#endif

namespace fptu {
namespace details {

constexpr inline uint16_t le16(const uint16_t *ptr) noexcept { return *ptr; }
constexpr inline uint32_t le32(const uint32_t *ptr) noexcept { return *ptr; }
constexpr inline uint64_t le64(const uint64_t *ptr) noexcept { return *ptr; }

constexpr inline void store_le16(uint16_t *ptr, uint16_t value) noexcept {
  *ptr = value;
}
constexpr inline void store_le32(uint32_t *ptr, uint32_t value) noexcept {
  *ptr = value;
}
constexpr inline void store_le64(uint64_t *ptr, uint64_t value) noexcept {
  *ptr = value;
}

} // namespace details
} // namespace fptu
