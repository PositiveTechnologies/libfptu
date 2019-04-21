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
#include "fast_positive/details/erthink/erthink_defs.h"

#ifdef __cplusplus

#include <climits>
#include <cstddef>
#include <cstring>
#include <string>

#if __cplusplus >= 201703L && __has_include(<string_view>)
#include <string_view>
#define HAVE_cxx17_std_string_view 1
#else
#define HAVE_cxx17_std_string_view 0
#endif

namespace fptu {

/* Минималистичное подобие std::string_view из C++17, но с отличиями:
 *  - более быстрое СРАВНЕНИЕ С ДРУГОЙ СЕМАНТИКОЙ, сначала учитывается длина!
 *  - отсутствуют сервисные методы: remove_prefix, remove_suffix, substr, copy,
 *    starts_with, ends_with, find, rfind, find_first_of, find_last_of,
 *    find_first_not_of, find_last_not_of, operator<<(std::ostream). */
class string_view {
protected:
  const char *str;
  intptr_t len /* LY: здесь намеренно используется intptr_t:
                   - со знаком для отличия нулевой длины от отсутствия
                     значения (nullptr), используя len == -1 как DENIL.
                   - не ptrdiff_t чтобы упростить оператор сравнения,
                     из которого не стоит возвращать ptrdiff_t. */
      ;

public:
  constexpr string_view() : str(nullptr), len(-1) {}
  constexpr string_view(const string_view &v) = default;
  cxx14_constexpr string_view &
  operator=(const string_view &v) noexcept = default;

  constexpr string_view(const char *str, std::size_t count)
      : str(str), len(str ? static_cast<intptr_t>(count) : -1) {
    constexpr_assert(len >= 0 || (len == -1 && !str));
  }

  constexpr string_view(const char *begin, const char *end)
      : str(begin), len(begin ? static_cast<intptr_t>(end - begin) : -1) {
    constexpr_assert(end >= begin);
    constexpr_assert(len >= 0 || (len == -1 && !begin));
  }

  constexpr string_view(const char *ptr)
      : str(ptr), len(ptr ? static_cast<intptr_t>(strlen(ptr)) : -1) {
    constexpr_assert(len >= 0 || (len == -1 && !str));
  }
  /* Конструктор из std::string ОБЯЗАН быть explicit для предотвращения
   * проблемы reference to temporary object из-за неявного создания string_view
   * из переданной по значению временного экземпляра std::string. */
  explicit /* не может быть constexpr из-за std::string::size() */ string_view(
      const std::string &s)
      : str(s.data()), len(static_cast<intptr_t>(s.size())) {
    assert(s.size() < npos);
  }
  operator std::string() const { return std::string(data(), length()); }

#if HAVE_cxx17_std_string_view
  /* Конструктор из std::string_view:
   *  - Может быть НЕ-explicit, так как у std::string_view нет неявного
   *    конструктора из std::string. Поэтому не возникает проблемы
   *    reference to temporary object из-за неявного создания string_view
   *    из переданной по значению временного экземпляра std::string.
   *  - НЕ ДОЛЖЕН быть explicit для бесшовной интеграции с std::string_view. */
  constexpr string_view(const std::string_view &v) noexcept
      : str(v.data()), len(static_cast<intptr_t>(v.size())) {
    assert(v.size() < npos);
  }
  constexpr operator std::string_view() const noexcept {
    return std::string_view(data(), length());
  }
  constexpr string_view &operator=(std::string_view &v) noexcept {
    assert(v.size() < npos);
    this->str = v.data();
    this->len = static_cast<intptr_t>(v.size());
    return *this;
  }
  constexpr void swap(std::string_view &v) noexcept {
    const auto temp = *this;
    *this = v;
    v = temp;
  }
#endif /* HAVE_cxx17_std_string_view */

  cxx14_constexpr void swap(string_view &v) noexcept {
    const auto temp = *this;
    *this = v;
    v = temp;
  }

  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef char value_type;
  typedef const char *const_pointer;
  typedef const char *pointer;
  typedef const char &const_reference;
  typedef const_reference reference;
  constexpr const_reference operator[](size_type pos) const { return str[pos]; }
  constexpr const_reference front() const {
    constexpr_assert(len > 0);
    return str[0];
  }
  constexpr const_reference back() const {
    constexpr_assert(len > 0);
    return str[len - 1];
  }
  const_reference at(size_type pos) const;
  static constexpr size_type npos = size_type(INT_MAX);

  typedef const char *const_iterator;
  constexpr const_iterator cbegin() const { return str; }
  constexpr const_iterator cend() const { return str + length(); }
  typedef const_iterator iterator;
  constexpr iterator begin() const { return cbegin(); }
  constexpr iterator end() const { return cend(); }

  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }
  typedef const_reverse_iterator reverse_iterator;
  reverse_iterator rbegin() const { return crbegin(); }
  reverse_iterator rend() const { return crend(); }

  constexpr const char *data() const { return str; }
  constexpr std::size_t length() const { return (len >= 0) ? (size_t)len : 0u; }
  constexpr bool empty() const { return len <= 0; }
  constexpr bool nil() const { return len < 0; }
  constexpr std::size_t size() const { return length(); }
  constexpr size_type max_size() const { return 32767; }

  cxx14_constexpr std::size_t hash_value() const {
    std::size_t h = (size_t)len * 3977471;
    for (intptr_t i = 0; i < len; ++i)
      h = (h ^ str[i]) * 1664525 + 1013904223;
    return h ^ 3863194411 * (h >> 11);
  }

  static cxx14_constexpr intptr_t compare(const string_view &a,
                                          const string_view &b) {
    const intptr_t diff = a.len - b.len;
    return diff ? diff
                : (a.str == b.str) ? 0 : memcmp(a.data(), b.data(), a.length());
  }
  cxx14_constexpr bool operator==(const string_view &v) const {
    return compare(*this, v) == 0;
  }
  cxx14_constexpr bool operator<(const string_view &v) const {
    return compare(*this, v) < 0;
  }
  cxx14_constexpr bool operator>(const string_view &v) const {
    return compare(*this, v) > 0;
  }
  cxx14_constexpr bool operator<=(const string_view &v) const {
    return compare(*this, v) <= 0;
  }
  cxx14_constexpr bool operator>=(const string_view &v) const {
    return compare(*this, v) >= 0;
  }
  cxx14_constexpr bool operator!=(const string_view &v) const {
    return compare(*this, v) != 0;
  }

  static /* не может быть constexpr из-за std::string::size() */ intptr_t
  compare(const std::string &a, const string_view &b) {
    return compare(string_view(a), b);
  }
  static /* не может быть constexpr из-за std::string::size() */ intptr_t
  compare(const string_view &a, const std::string &b) {
    return compare(a, string_view(b));
  }
  bool operator==(const std::string &s) const { return compare(*this, s) == 0; }
  bool operator<(const std::string &s) const { return compare(*this, s) < 0; }
  bool operator>(const std::string &s) const { return compare(*this, s) > 0; }
  bool operator<=(const std::string &s) const { return compare(*this, s) <= 0; }
  bool operator>=(const std::string &s) const { return compare(*this, s) >= 0; }
  bool operator!=(const std::string &s) const { return compare(*this, s) != 0; }
};
} // namespace fptu

namespace std {
template <> struct hash<fptu::string_view> {
  cxx14_constexpr std::size_t operator()(fptu::string_view const &v) const {
    return v.hash_value();
  }
};
} // namespace std

inline bool operator>(const fptu::string_view &a, const std::string &b) {
  return fptu::string_view::compare(a, b) > 0;
}
inline bool operator>=(const fptu::string_view &a, const std::string &b) {
  return fptu::string_view::compare(a, b) >= 0;
}
inline bool operator<(const fptu::string_view &a, const std::string &b) {
  return fptu::string_view::compare(a, b) < 0;
}
inline bool operator<=(const fptu::string_view &a, const std::string &b) {
  return fptu::string_view::compare(a, b) <= 0;
}
inline bool operator==(const fptu::string_view &a, const std::string &b) {
  return fptu::string_view::compare(a, b) == 0;
}
inline bool operator!=(const fptu::string_view &a, const std::string &b) {
  return fptu::string_view::compare(a, b) != 0;
}

inline bool operator>(const std::string &a, const fptu::string_view &b) {
  return fptu::string_view::compare(a, b) > 0;
}
inline bool operator>=(const std::string &a, const fptu::string_view &b) {
  return fptu::string_view::compare(a, b) >= 0;
}
inline bool operator<(const std::string &a, const fptu::string_view &b) {
  return fptu::string_view::compare(a, b) < 0;
}
inline bool operator<=(const std::string &a, const fptu::string_view &b) {
  return fptu::string_view::compare(a, b) <= 0;
}
inline bool operator==(const std::string &a, const fptu::string_view &b) {
  return fptu::string_view::compare(a, b) == 0;
}
inline bool operator!=(const std::string &a, const fptu::string_view &b) {
  return fptu::string_view::compare(a, b) != 0;
}

#endif /* __cplusplus */
