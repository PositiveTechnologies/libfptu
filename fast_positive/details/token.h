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
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/meta.h"
#include "fast_positive/details/type2genus.h"

#include <type_traits> // for std::is_standard_layout

namespace fptu {
namespace details {

template <typename> struct token_operations;
template <tag_t> struct token_static;

template <typename TOKEN> struct token_operations : public TOKEN {
  constexpr token_operations() noexcept = default;
  constexpr token_operations(const token_operations &) noexcept = default;
  constexpr token_operations(const tag_t tag) noexcept : TOKEN(tag) {}

  constexpr tag_t tag() const noexcept { return TOKEN::tag(); }
  constexpr genus type() const noexcept { return details::tag2genus(tag()); }
  constexpr bool is_valid() const noexcept { return type() != genus::hole; }
  constexpr unsigned id() const noexcept { return details::tag2id(tag()); }

  constexpr bool operator==(const TOKEN &ditto) const noexcept {
    return tag() == ditto.tag();
  }
  constexpr bool operator!=(const TOKEN &ditto) const noexcept {
    return tag() != ditto.tag();
  }

  constexpr ptrdiff_t preplaced_offset() const noexcept {
    return details::tag2offset(tag());
  }

  constexpr bool is_saturated() const noexcept {
    return details::is_saturated(tag());
  }
  constexpr bool is_rangechecking() const noexcept {
    return details::is_rangechecking(tag());
  }

  constexpr bool is_preplaced() const noexcept {
    return details::is_preplaced(tag());
  }
  constexpr bool distinct_null() const noexcept {
    return is_preplaced() && details::distinct_null(tag());
  }

  constexpr bool is_loose() const noexcept { return details::is_loose(tag()); }
  constexpr bool is_inlay() const noexcept {
    return details::is_loose_inlay(tag());
  }
  constexpr bool is_collection() const noexcept {
    return details::is_loose_collection(tag());
  }
  constexpr bool is_quietabsence() const noexcept {
    return /*is_loose() &&*/ details::is_quietabsence(tag());
  }

  //----------------------------------------------------------------------------

  // constexpr bool is_varlen() const { return ; }
  // constexpr bool is_nested() const { return ; }

  constexpr bool is_bool() const noexcept { return type() == boolean; }
  constexpr bool is_enum() const noexcept { return type() == enumeration; }
  constexpr bool is_text() const noexcept { return type() == text; }
  constexpr bool is_number() const noexcept {
    return utils::test_bit(mask_number, type());
  }
  constexpr bool is_integer() const noexcept {
    return utils::test_bit(mask_integer, type());
  }
  constexpr bool is_signed() const noexcept {
    return utils::test_bit(mask_signed, type());
  }
  constexpr bool is_unsigned() const noexcept {
    return utils::test_bit(mask_unsigned, type());
  }
  constexpr bool is_float() const noexcept {
    return utils::test_bit(mask_float, type());
  }
  constexpr bool is_decimal() const noexcept { return type() == d64; }
};

struct token_static_tag {
  using is_static_token = std::true_type;
};

class token_nonstatic_tag {
  tag_t tag_;

protected:
  constexpr token_nonstatic_tag(const tag_t tag) noexcept : tag_(tag) {}

public:
  using is_static_token = std::false_type;
  static constexpr bool is_static_preplaced() noexcept { return false; }
  constexpr tag_t tag() const noexcept { return tag_; }
  constexpr token_nonstatic_tag() noexcept : tag_(~UINT32_C(0)) {
    static_assert(sizeof(tag_) == 4, "WTF?");
  }
  constexpr token_nonstatic_tag(const token_nonstatic_tag &) noexcept = default;

  constexpr bool operator==(const token_nonstatic_tag &ditto) const noexcept {
    return tag() == ditto.tag();
  }
  constexpr bool operator!=(const token_nonstatic_tag &ditto) const noexcept {
    return tag() != ditto.tag();
  }
};

template <tag_t TAG> struct token_static : public token_static_tag {
  static constexpr tag_t static_tag = TAG;
  static constexpr genus static_genus = tag2genus(TAG);
  static constexpr bool is_static_preplaced() noexcept {
    return is_preplaced(TAG);
  }
  static constexpr ptrdiff_t static_offset =
      is_preplaced(TAG) ? tag2offset(TAG) : PTRDIFF_MAX;
  using traits = meta::genus_traits<static_genus>;
  constexpr tag_t tag() const noexcept { return TAG; }

protected:
  cxx14_constexpr token_static(const tag_t tag) noexcept {
    constexpr_assert(tag == TAG);
    (void)tag;
  }
  constexpr token_static() noexcept = default;

  constexpr bool operator==(const token_nonstatic_tag &ditto) const noexcept {
    return tag() == ditto.tag();
  }
  constexpr bool operator!=(const token_nonstatic_tag &ditto) const noexcept {
    return tag() != ditto.tag();
  }
};

} // namespace details

namespace details {
template <class T> struct remove_member_pointer;

template <class STRUCT_TYPE, class FIELD_TYPE>
struct remove_member_pointer<FIELD_TYPE STRUCT_TYPE::*> {
  using type = FIELD_TYPE;
};

template <class T> struct baseof_member_pointer;

template <class STRUCT_TYPE, class FIELD_TYPE>
struct baseof_member_pointer<FIELD_TYPE STRUCT_TYPE::*> {
  using type = STRUCT_TYPE;
};

template <typename STRUCT_TYPE, typename FIELD_TYPE,
          FIELD_TYPE STRUCT_TYPE::*FIELD, std::size_t OFFSET,
          bool DENILED = false, bool SATURATED = false>
class token_native
    : public token_operations<token_static<details::tag_from_offset(
          OFFSET, meta::type2genus<FIELD_TYPE>::value, sizeof(FIELD_TYPE),
          DENILED, SATURATED)>> {
public:
  constexpr token_native() noexcept {
    static_assert(std::is_standard_layout<STRUCT_TYPE>::value,
                  "Constraint is violated");
    static_assert(std::is_pod<FIELD_TYPE>::value, "Constraint is violated");
  }
};

} // namespace details

#define FPTU_TEMPLATE_FOR_STATIC_TOKEN                                         \
  template <                                                                   \
      typename TOKEN,                                                          \
      typename erthink::enable_if_t<                                           \
          std::is_base_of<::fptu::details::token_static_tag, TOKEN>::value &&  \
          TOKEN::is_static_token::value> * = nullptr>

//------------------------------------------------------------------------------

class token : public details::token_operations<details::token_nonstatic_tag> {
  using base = details::token_operations<details::token_nonstatic_tag>;
  explicit constexpr token(const details::tag_t tag) noexcept : base(tag) {}

  static genus validate_loose_type(const genus type) {
    if (unlikely(type >= genus::hole))
      throw_invalid_argument("type >= fptu::genus::hole");
    return type;
  }
  static unsigned validate_loose_id(const unsigned id) {
    if (unlikely(id > details::tag_bits::max_ident))
      throw_invalid_argument("id > fptu::details::max_ident");
    return id;
  }
  static genus validate_preplaced_type(const genus type) {
    if (unlikely(type > genus::hole))
      throw_invalid_argument("type > fptu::genus::hole");
    return type;
  }
  static std::size_t validate_preplaced_offset(const ptrdiff_t offset) {
    if (unlikely(std::size_t(offset) > details::max_preplaced_offset))
      throw_invalid_argument(
          "offset < 0 || offset > details::max_preplaced_offset");
    return std::size_t(offset);
  }

public:
  constexpr token() noexcept : base() {}

  token(const genus type, const unsigned id, const bool collection = false,
        const bool quietabsence = false, const bool saturated = false)
      : base(details::make_tag(validate_loose_type(type), validate_loose_id(id),
                               collection, quietabsence, saturated)) {}

  token(const ptrdiff_t offset, const genus type, const bool deniled = false,
        const bool saturated = false)
      : base(details::tag_from_offset(
            validate_preplaced_offset(offset), validate_preplaced_type(type),
            details::preplaced_bytes(type), deniled, saturated)) {}
  constexpr token(const token &) noexcept = default;
  constexpr token(const details::token_nonstatic_tag &other) noexcept
      : token(other.tag()) {}

  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  explicit constexpr token(const TOKEN &) noexcept : token(TOKEN::static_tag) {}
};

template <genus TYPE, class TOKEN> class cast_typecheck : public TOKEN {
  cast_typecheck() = delete;

public:
  cxx14_constexpr cast_typecheck(const TOKEN &token) : TOKEN(token) {
    if (unlikely(token.type() != TYPE))
      throw_type_mismatch();
  }
  constexpr genus type() const noexcept { return TYPE; }
};

template <class TOKEN> class cast_preplaced : public TOKEN {
  cast_preplaced() = delete;

public:
  cxx14_constexpr cast_preplaced(const TOKEN &token) : TOKEN(token) {
    if (unlikely(!token.is_preplaced()))
      throw_type_mismatch();
  }
  constexpr bool is_preplaced() const noexcept { return true; }
  constexpr bool is_loose() const noexcept { return false; }
  constexpr bool is_inlay() const noexcept { return false; }
  constexpr bool is_collection() const noexcept { return false; }
};

template <class TOKEN> class cast_loose : public TOKEN {
  cast_loose() = delete;

public:
  cxx14_constexpr cast_loose(const TOKEN &token) : TOKEN(token) {
    if (unlikely(!token.is_loose()))
      throw_type_mismatch();
  }
  constexpr bool is_preplaced() const noexcept { return false; }
  constexpr bool is_loose() const noexcept { return true; }
};

using token_preplaced = cast_preplaced<token>;
using token_loose = cast_loose<token>;

} // namespace fptu

//------------------------------------------------------------------------------

//
//  struct Foo {
//    bool Bar;
//  };
//
//  typedef FPTU_TOKEN(Foo, Bar) MyToken_FooBar;
//
#define FPTU_TOKEN(STRUCT, FIELD)                                              \
  ::fptu::details::token_native<                                               \
      ::fptu::details::baseof_member_pointer<decltype(&STRUCT::FIELD)>::type,  \
      ::fptu::details::remove_member_pointer<decltype(&STRUCT::FIELD)>::type,  \
      &STRUCT::FIELD, offsetof(STRUCT, FIELD)>
