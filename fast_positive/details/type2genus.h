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

/*
 * libfptu = { Fast Positive Tuples, aka Позитивные Кортежи }
 *
 * The kind of lightweight linearized tuples, which are extremely handy
 * to machining, including cases with shared memory.
 * Please see README.md at https://github.com/leo-yuriev/libfptu
 *
 * The Future will Positive. Всё будет хорошо.
 *
 * "Позитивные Кортежи" дают легковесное линейное представление небольших
 * JSON-подобных структур в экстремально удобной для машины форме,
 * в том числе при размещении в разделяемой памяти.
 */

#pragma once
#include "fast_positive/details/api.h"
#include "fast_positive/details/essentials.h"

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
FPTU_DECLARE_TYPE2GENUS(uuid_t, genus::b128);

FPTU_DECLARE_TYPE2GENUS(std::string, genus::text);
FPTU_DECLARE_TYPE2GENUS(string_view, genus::text);

#undef FPTU_DECLARE_TYPE2GENUS

//------------------------------------------------------------------------------

// Primary template for type2shape.
template <typename TYPE> struct type2shape;

} // namespace meta

} // namespace fptu
