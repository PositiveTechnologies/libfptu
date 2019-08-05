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
#ifdef __cplusplus

#include "fast_positive/erthink/erthink_defs.h"
#include <cstddef>
#include <cstdint>

namespace fptu {

namespace utils {

template <typename T> class range {
  T *const begin_;
  T *const end_;

public:
  constexpr range() noexcept : begin_(nullptr), end_(nullptr) {}
  constexpr range(T *begin, T *end) noexcept : begin_(begin), end_(end) {
    constexpr_assert(end >= begin);
  }
  constexpr range(T *begin, std::size_t count) noexcept
      : begin_(begin), end_(begin + count) {
    constexpr_assert(end_ >= begin_);
  }
  constexpr range(const range &) noexcept = default;
  range &operator=(const range &) noexcept = default;
  constexpr T *begin() const noexcept { return begin_; }
  constexpr T *end() const noexcept { return end_; }
  constexpr bool empty() const noexcept { return begin_ == end_; }
  constexpr std::size_t size() const noexcept { return end_ - begin_; }
};

//------------------------------------------------------------------------------

template <unsigned... Items> struct bitset_mask;

template <unsigned One> struct bitset_mask<One> {
  static constexpr uintptr_t value = uintptr_t(1) << One;
};

template <unsigned First, unsigned... Rest> struct bitset_mask<First, Rest...> {
  static constexpr uintptr_t value =
      bitset_mask<First>::value | bitset_mask<Rest...>::value;
};

template <typename MASK, typename BIT>
constexpr bool test_bit(const MASK mask, const BIT bit) {
  return (mask & (MASK(1) << bit)) != 0;
}

//------------------------------------------------------------------------------

template <size_t bytes> constexpr intptr_t bytes_disjunction(const void *ptr);

template <> constexpr intptr_t bytes_disjunction<1>(const void *ptr) {
  return *static_cast<const int8_t *>(ptr);
}

template <> constexpr intptr_t bytes_disjunction<2>(const void *ptr) {
  return *static_cast<const int16_t *>(ptr);
}

template <> constexpr intptr_t bytes_disjunction<3>(const void *ptr) {
  return static_cast<const int16_t *>(ptr)[0] |
         static_cast<const int8_t *>(ptr)[2];
}

template <> constexpr intptr_t bytes_disjunction<4>(const void *ptr) {
  return *static_cast<const int32_t *>(ptr);
}

template <> constexpr intptr_t bytes_disjunction<5>(const void *ptr) {
  return static_cast<const int32_t *>(ptr)[0] |
         static_cast<const int8_t *>(ptr)[4];
}

template <> constexpr intptr_t bytes_disjunction<6>(const void *ptr) {
  return static_cast<const int32_t *>(ptr)[0] |
         static_cast<const int16_t *>(ptr)[2];
}

template <> constexpr intptr_t bytes_disjunction<7>(const void *ptr) {
  return static_cast<const int32_t *>(ptr)[0] |
         static_cast<const int16_t *>(ptr)[2] |
         static_cast<const int8_t *>(ptr)[6];
}

template <> constexpr intptr_t bytes_disjunction<8>(const void *ptr) {
  return (sizeof(intptr_t) == 8) ? *static_cast<const intptr_t *>(ptr)
                                 : static_cast<const int32_t *>(ptr)[0] |
                                       static_cast<const int32_t *>(ptr)[1];
}

template <size_t bytes> constexpr intptr_t bytes_disjunction(const void *ptr) {
  static_assert(bytes > 8, "WTF?");
  return bytes_disjunction<8>(ptr) |
         bytes_disjunction<bytes - 8>(static_cast<const int8_t *>(ptr) + 8);
}

//------------------------------------------------------------------------------

template <size_t bytes> constexpr intptr_t bytes_conjunction(const void *ptr);

template <> constexpr intptr_t bytes_conjunction<1>(const void *ptr) {
  return *static_cast<const int8_t *>(ptr);
}

template <> constexpr intptr_t bytes_conjunction<2>(const void *ptr) {
  return *static_cast<const int16_t *>(ptr);
}

template <> constexpr intptr_t bytes_conjunction<3>(const void *ptr) {
  return static_cast<const int16_t *>(ptr)[0] &
         static_cast<const int8_t *>(ptr)[2];
}

template <> constexpr intptr_t bytes_conjunction<4>(const void *ptr) {
  return *static_cast<const int32_t *>(ptr);
}

template <> constexpr intptr_t bytes_conjunction<5>(const void *ptr) {
  return static_cast<const int32_t *>(ptr)[0] &
         static_cast<const int8_t *>(ptr)[4];
}

template <> constexpr intptr_t bytes_conjunction<6>(const void *ptr) {
  return static_cast<const int32_t *>(ptr)[0] &
         static_cast<const int16_t *>(ptr)[2];
}

template <> constexpr intptr_t bytes_conjunction<7>(const void *ptr) {
  return static_cast<const int32_t *>(ptr)[0] &
         static_cast<const int16_t *>(ptr)[2] &
         static_cast<const int8_t *>(ptr)[6];
}

template <> constexpr intptr_t bytes_conjunction<8>(const void *ptr) {
  return (sizeof(intptr_t) == 8) ? *static_cast<const intptr_t *>(ptr)
                                 : static_cast<const int32_t *>(ptr)[0] &
                                       static_cast<const int32_t *>(ptr)[1];
}

template <size_t bytes> constexpr intptr_t bytes_conjunction(const void *ptr) {
  static_assert(bytes > 8, "WTF?");
  return bytes_conjunction<8>(ptr) &
         bytes_conjunction<bytes - 8>(static_cast<const int8_t *>(ptr) + 8);
}

//------------------------------------------------------------------------------

template <size_t bytes> constexpr bool is_zero(const void *ptr) {
  return bytes_disjunction<bytes>(ptr) == 0;
}

template <size_t bytes> constexpr bool is_allones(const void *ptr) {
  return bytes_conjunction<bytes>(ptr) == intptr_t(-1);
}

//------------------------------------------------------------------------------

template <typename INTEGER> constexpr bool is_power2(INTEGER value) noexcept {
  return (value & (value - 1)) == 0 && value > 0;
}

template <typename TYPE, intptr_t align>
constexpr bool is_aligned(const TYPE *ptr) noexcept {
  static_assert(is_power2(align), "WTF?");
  return ((align - 1) & reinterpret_cast<intptr_t>(ptr)) == 0;
}

template <typename INTEGER, intptr_t align>
constexpr INTEGER floor(INTEGER value) noexcept {
  constexpr_assert(is_power2(align));
  return value & ~(align - 1);
}

template <typename INTEGER>
constexpr INTEGER ceil(INTEGER value, intptr_t align) noexcept {
  constexpr_assert(is_power2(align));
  return (value + align - 1) & ~(align - 1);
}

} // namespace utils
} // namespace fptu

#endif /* __cplusplus */
