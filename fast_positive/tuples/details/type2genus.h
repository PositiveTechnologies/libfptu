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
#include "fast_positive/tuples/details/field.h"
#include "fast_positive/tuples/essentials.h"

#include <type_traits>

namespace fptu {
namespace meta {

// Primary template for type2genus.
template <typename TYPE> struct type2genus;

//------------------------------------------------------------------------------

// Helper struct for SFINAE-poisoning type2genus.
template <typename TYPE, bool validator> struct __type2genus_poison {
private:
  genus static inline constexpr static_assert_helper() {
    static_assert(validator, "should be known or fundamental/native type");
    return genus::hole;
  }
  // Private rather than deleted to be non-trivially-copyable.
  __type2genus_poison(__type2genus_poison &&);
  ~__type2genus_poison();

public:
  static constexpr genus value = static_assert_helper();
};

//------------------------------------------------------------------------------

// Helper struct for SFINAE-poisoning non-enum types.
template <typename TYPE, bool = std::is_enum<TYPE>::value>
struct __type2genus_enum {
private:
  // Private rather than deleted to be non-trivially-copyable.
  __type2genus_enum(__type2genus_enum &&);
  ~__type2genus_enum();
};

// Helper struct for enum types.
template <typename TYPE> struct __type2genus_enum<TYPE, true> {
private:
  genus static inline constexpr static_assert_helper() {
    static_assert(std::is_enum<TYPE>::value, "must be enum type");
    return genus::enumeration;
  }

public:
  static constexpr genus value = static_assert_helper();
};

// Primary template for enum types only, Use with non-enum types still SFINAES.
template <typename TYPE> struct type2genus : __type2genus_enum<TYPE> {};

//------------------------------------------------------------------------------

#define FPTU_DECLARE_TYPE2GENUS(type, tag)                                     \
  template <> struct type2genus<type> { static constexpr genus value = tag; }

FPTU_DECLARE_TYPE2GENUS(bool, genus::boolean);
FPTU_DECLARE_TYPE2GENUS(int8_t, genus::i8);
FPTU_DECLARE_TYPE2GENUS(uint8_t, genus::u8);
FPTU_DECLARE_TYPE2GENUS(int16_t, genus::i16);
FPTU_DECLARE_TYPE2GENUS(uint16_t, genus::u16);

FPTU_DECLARE_TYPE2GENUS(int32_t, genus::i32);
FPTU_DECLARE_TYPE2GENUS(uint32_t, genus::u32);
FPTU_DECLARE_TYPE2GENUS(float, genus::f32);

FPTU_DECLARE_TYPE2GENUS(int64_t, genus::i64);
FPTU_DECLARE_TYPE2GENUS(uint64_t, genus::u64);
FPTU_DECLARE_TYPE2GENUS(double, genus::f64);
FPTU_DECLARE_TYPE2GENUS(datetime_t, genus::t64);
FPTU_DECLARE_TYPE2GENUS(decimal64, genus::d64);

FPTU_DECLARE_TYPE2GENUS(mac_address_t, genus::mac);
FPTU_DECLARE_TYPE2GENUS(ip_address_t, genus::ip);
FPTU_DECLARE_TYPE2GENUS(uuid_t, genus::bin128);

FPTU_DECLARE_TYPE2GENUS(preplaced_string, genus::text);
FPTU_DECLARE_TYPE2GENUS(preplaced_varbin, genus::varbin);
FPTU_DECLARE_TYPE2GENUS(preplaced_nested, genus::nested);
FPTU_DECLARE_TYPE2GENUS(preplaced_property, genus::property);

FPTU_DECLARE_TYPE2GENUS(binary96_t, genus::bin96);
FPTU_DECLARE_TYPE2GENUS(binary128_t, genus::bin128);
FPTU_DECLARE_TYPE2GENUS(binary160_t, genus::bin160);
FPTU_DECLARE_TYPE2GENUS(binary192_t, genus::bin192);
FPTU_DECLARE_TYPE2GENUS(binary224_t, genus::bin224);
FPTU_DECLARE_TYPE2GENUS(binary256_t, genus::bin256);
FPTU_DECLARE_TYPE2GENUS(binary320_t, genus::bin320);
FPTU_DECLARE_TYPE2GENUS(binary384_t, genus::bin384);
FPTU_DECLARE_TYPE2GENUS(binary512_t, genus::bin512);

#undef FPTU_DECLARE_TYPE2GENUS

//------------------------------------------------------------------------------

// Primary template for type2shape.
template <typename TYPE> struct type2shape;

} // namespace meta

} // namespace fptu
