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
#include "fast_positive/tuples/details/meta.h"
#include "fast_positive/tuples/details/type2genus.h"
#include "fast_positive/tuples/essentials.h"

#include <type_traits> // for std::is_standard_layout

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4275) /* non dll-interface class 'FOO' used as base  \
                                 * for dll-interface class 'BAR'               \
                                 * (FALSE-POSITIVE in the MOST CASES). */
#endif

namespace fptu {

template <details::tag_t> struct token_static;

namespace details {

template <typename> struct token_operations;

template <typename TOKEN> struct token_operations : public TOKEN {
  constexpr token_operations() noexcept = default;
  constexpr token_operations(const token_operations &) noexcept = default;
  constexpr token_operations(const tag_t tag) noexcept : TOKEN(tag) {}

  constexpr tag_t tag() const noexcept { return TOKEN::tag(); }
  constexpr genus type() const noexcept { return details::tag2genus(tag()); }
  constexpr bool is_valid() const noexcept { return tag() != 0; }
  constexpr unsigned id() const noexcept { return details::tag2id(tag()); }

  constexpr ptrdiff_t preplaced_offset() const noexcept {
    return details::tag2offset(tag());
  }

  constexpr size_t preplaced_size() const noexcept {
    return details::tag2indysize(tag());
  }

  constexpr bool is_saturated() const noexcept {
    return details::is_saturated(tag());
  }
  constexpr bool is_rangechecking() const noexcept { return !is_saturated(); }

  constexpr bool is_preplaced() const noexcept {
    return details::is_preplaced(tag());
  }

  constexpr bool is_discernible_null() const noexcept {
    /* Ради упрощения и максимальной ясности решено оставить один
     * флажок/параметр определяющий поведение для работы с NULL-значениями.
     *
     * Когда discernible_null == TRUE:
     *   - для preplaced-полей фиксированного размера поддерживаются
     *     фиксированные предопределенные DENIL-значения;
     *   - для loose-полей фиксированного размера какая-либо дополнительная
     *     обработка не производится;
     *   - для любых полей переменной длины пустые значения (строки нулевой
     *     длины) сохраняются в неизменном виде, тем самым обеспечивается
     *     отличие пустых значений от NULL (отсутствие полей или значений).
     *   - при попытке чтения логически или физически отсутствующих полей
     *     вбрасывается исключение fptu::field_absent.
     *
     * Когда discernible_null == FALSE:
     *   - для preplaced-полей фиксированного размера какая-либо дополнительная
     *     обработка не производится, т.е. читается "как есть";
     *   - для loose-полей фиксированного размера запись значения
     *     соответствующего естественному нулю приводит к удалению поля.
     *   - для любых полей переменной длины запись пустого значения (строки
     *     нулевой длины приводит к удалению поля или значения), т.е. пустые
     *     значения преобразуются в NULL;
     *   - при попытке чтении отсутствующего поля или отсутствующего значения
     *     подставляется пустое значение (пустая строка или естественный ноль).
     *
     * Таким образом, при discernible_null == TRUE приложение может видеть
     * разницу между NULL и нулевым значением, а при discernible_null == FALSE
     * внешне различия не видны (но нули и пустые значения не хранятся).
     *
     * -------------------+-------------------------+-------------------------
     *                    | Отличать NULL,          | НЕ отличать NULL,
     *                    | throw field_absent()    | подставлять 0/пусто
     * -------------------+-------------------------+-------------------------
     *  Fixed Preplaced   | Write(DENIL) => Remove, | Write/Read "AS IS",
     *                    | Read(DENIL) => Throw    | No NULLs
     * -------------------+-------------------------+-------------------------
     *  Fixed Loose       | Write("AS IS"),         | Write(0) => Remove,
     *                    | Read(NULL) => Throw     | Read(NULL) => 0
     * -------------------+-------------------------+-------------------------
     *  Stretchy          | Write("AS IS"),         | Write(EMPTY) => Remove,
     *  Preplaced & Loose | Read(NULL) => Throw     | Read(NULL) => EMPTY
     * -------------------+-------------------------+-------------------------
     *
     * Значения для designated_null зафиксированы в коде libfptu,
     * а не задаются схемой. Причин для такого решения две:
     *  - При разборе ситуаций не нашлось случаев когда designated_null
     *    действительно требовался, а выбранное зарезервированное значение
     *    не подходило.
     *  - При кастомизации denil-значений их невозможно "приклеить" к
     *    легковесным токенам, а придёться определять в схеме. Это не только
     *    усложняет схему, но и требует взаимодействия с ней при каждом чтении
     *    значений полей, т.е. токенов будет уже недостаточно для взаимодействия
     *    с кортежами. */
    return details::is_discernible_null(tag());
  }

  constexpr bool is_loose() const noexcept { return details::is_loose(tag()); }
  constexpr bool is_inlay() const noexcept {
    return details::is_loose_inlay(tag());
  }
  constexpr bool is_collection() const noexcept {
    return details::is_loose_collection(tag());
  }

  //----------------------------------------------------------------------------

  constexpr bool is_stretchy() const noexcept {
    return !details::is_fixed_size(tag());
  }

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
  constexpr token_nonstatic_tag() noexcept : tag_(0) {
    static_assert(sizeof(tag_) == 4, "WTF?");
  }
  constexpr token_nonstatic_tag(const token_nonstatic_tag &) noexcept = default;

  void enforce_discernible_null(bool value) {
    tag_ = value ? tag_ | tag_bits::discernible_null_flag
                 : tag_ & ~tag_t(tag_bits::discernible_null_flag);
  }

  void enforce_saturation(bool value) {
    tag_ = value ? tag_ | tag_bits::saturation_flag
                 : tag_ & ~tag_t(tag_bits::saturation_flag);
  }

  friend constexpr bool operator==(const token_nonstatic_tag &a,
                                   const token_nonstatic_tag &b) noexcept {
    return a.tag() == b.tag();
  }
  friend constexpr bool operator!=(const token_nonstatic_tag &a,
                                   const token_nonstatic_tag &b) noexcept {
    return !(a == b);
  }
  friend constexpr bool operator<(const token_nonstatic_tag &a,
                                  const token_nonstatic_tag &b) noexcept {
    return details::tag_less(a.tag(), b.tag());
  }
  friend constexpr bool operator>=(const token_nonstatic_tag &a,
                                   const token_nonstatic_tag &b) noexcept {
    return !(a < b);
  }
  friend constexpr bool operator>(const token_nonstatic_tag &a,
                                  const token_nonstatic_tag &b) noexcept {
    return b < a;
  }
  friend constexpr bool operator<=(const token_nonstatic_tag &a,
                                   const token_nonstatic_tag &b) noexcept {
    return !(a > b);
  }

  struct hash {
    constexpr std::size_t operator()(const token_nonstatic_tag &ident) const
        noexcept {
      const auto m = ident.tag() * size_t(2709533891);
      return m ^ (m >> 19);
    }
  };

  constexpr static bool is_same(const token_nonstatic_tag &a,
                                const token_nonstatic_tag &b) noexcept {
    return details::tag_same(a.tag(), b.tag());
  }

  struct same {
    constexpr bool operator()(const token_nonstatic_tag &a,
                              const token_nonstatic_tag &b) const noexcept {
      return is_same(a, b);
    }
  };
};

} // namespace details

template <details::tag_t TAG>
struct token_static : public details::token_static_tag {
  static constexpr details::tag_t static_tag = TAG;
  static constexpr genus static_genus = details::tag2genus(TAG);
  static constexpr bool is_static_preplaced() noexcept {
    return details::is_preplaced(TAG);
  }
  static constexpr ptrdiff_t static_offset =
      details::is_preplaced(TAG) ? details::tag2offset(TAG) : PTRDIFF_MAX;
  using traits = meta::genus_traits<static_genus>;
  constexpr details::tag_t tag() const noexcept { return TAG; }

protected:
  cxx14_constexpr token_static(const details::tag_t tag) noexcept {
    constexpr_assert(tag == TAG);
    (void)tag;
  }
  constexpr token_static() noexcept = default;

  constexpr bool operator==(const details::token_nonstatic_tag &ditto) const
      noexcept {
    return tag() == ditto.tag();
  }
  constexpr bool operator!=(const details::token_nonstatic_tag &ditto) const
      noexcept {
    return tag() != ditto.tag();
  }
};

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
          bool DISCERNIBLE_NULL = false, bool SATURATED = false>
class token_native
    : public token_operations<token_static<details::tag_from_offset(
          OFFSET, meta::type2genus<FIELD_TYPE>::value, sizeof(FIELD_TYPE),
          DISCERNIBLE_NULL, SATURATED)>> {
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

class FPTU_API_TYPE token
    : public details::token_operations<details::token_nonstatic_tag> {
  using base = details::token_operations<details::token_nonstatic_tag>;
  explicit constexpr token(const details::tag_t tag) noexcept : base(tag) {}

  static cxx14_constexpr genus validate_loose_type(const genus type) {
    if (unlikely(type >= genus::hole))
      throw_invalid_argument("type >= fptu::genus::hole");
    return type;
  }
  static cxx14_constexpr unsigned validate_loose_id(const unsigned id) {
    if (unlikely(id > details::tag_bits::max_ident))
      throw_invalid_argument("id > fptu::details::max_ident");
    return id;
  }
  static cxx14_constexpr genus validate_preplaced_type(const genus type) {
    if (unlikely(type > genus::hole))
      throw_invalid_argument("type > fptu::genus::hole");
    return type;
  }
  static cxx14_constexpr std::size_t
  validate_preplaced_offset(const ptrdiff_t offset) {
    if (unlikely(std::size_t(offset) > details::max_preplaced_offset))
      throw_invalid_argument(
          "offset < 0 || offset > details::max_preplaced_offset");
    return std::size_t(offset);
  }

public:
  constexpr token() noexcept : base() {}

  cxx14_constexpr
  token(const genus type, const unsigned id, const bool collection = false,
        const bool discernible_null = false, const bool saturated = false)
      : base(details::make_tag(validate_loose_type(type), validate_loose_id(id),
                               collection, discernible_null, saturated)) {}

  cxx14_constexpr token(const ptrdiff_t offset, const genus type,
                        const bool discernible_null = false,
                        const bool saturated = false)
      : base(details::tag_from_offset(
            validate_preplaced_offset(offset), validate_preplaced_type(type),
            details::preplaced_bytes(type), discernible_null, saturated)) {}
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

//  namespace std {
//  template <>
//  struct hash<fptu::details::token_nonstatic_tag>
//     : public fptu::details::token_nonstatic_tag::hash {};
//  template <>
//  struct hash<fptu::token> : public fptu::details::token_nonstatic_tag::hash
//  {}; } // namespace std

#ifdef _MSC_VER
#pragma warning(pop)
#endif

//------------------------------------------------------------------------------

#define FPTU_TOKEN(STRUCT, FIELD)                                              \
  ::fptu::details::token_native<                                               \
      ::fptu::details::baseof_member_pointer<decltype(&STRUCT::FIELD)>::type,  \
      ::fptu::details::remove_member_pointer<decltype(&STRUCT::FIELD)>::type,  \
      &STRUCT::FIELD, offsetof(STRUCT, FIELD)>

// Пример использования макроса FPTU_TOKEN:
//
//  struct Foo {
//    bool Bar;
//    fptu::preplaced_string String;
//    fptu::preplaced_varbin Varbin;
//    fptu::preplaced_nested NestedTuple;
//    fptu::preplaced_property Property;
//  };
//
//  typedef FPTU_TOKEN(Foo, Bar) MyToken_FooBar;
//  typedef FPTU_TOKEN(Foo, String) MyToken_FooString;
//  typedef FPTU_TOKEN(Foo, Varbin) MyToken_FooVarbin;
//  typedef FPTU_TOKEN(Foo, NestedTuple) MyToken_FooNestedTuple;
//  typedef FPTU_TOKEN(Foo, Property) MyToken_FooProperty;
//
//  ...
//    bool bar_value = tuple.get_bool(MyToken_FooBar());
//    string_view string_value = tuple.get_bool(MyToken_FooString());
//
//  ...
//    Foo *foo = ... ;
//    string_view string_value = foo->String.value();
//
