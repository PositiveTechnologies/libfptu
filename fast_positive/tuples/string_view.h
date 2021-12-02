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
#include "fast_positive/erthink/erthink_defs.h"
#include "fast_positive/tuples/api.h"

#ifdef __cplusplus

#include <climits>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <string>

#if defined(__cpp_lib_string_view) && __cpp_lib_string_view >= 201606L
#include <string_view>
#define HAVE_std_string_view 1
#else
#define HAVE_std_string_view 0
#endif

namespace fptu {

/* Минималистичное подобие std::string_view из C++17, но с отличиями:
 *  - более быстрое СРАВНЕНИЕ С ДРУГОЙ СЕМАНТИКОЙ, сначала учитывается длина!
 *  - отсутствуют сервисные методы: remove_prefix, remove_suffix, substr, copy,
 *    starts_with, ends_with, find, rfind, find_first_of, find_last_of,
 *    find_first_not_of, find_last_not_of, operator<<(std::ostream). */
class FPTU_API_TYPE string_view {
protected:
  const char *str;
  intptr_t len /* LY: здесь намеренно используется intptr_t:
                   - со знаком для отличия нулевой длины от отсутствия
                     значения (nullptr), используя len == -1 как DENIL.
                   - не ptrdiff_t чтобы упростить оператор сравнения,
                     из которого не стоит возвращать ptrdiff_t. */
      ;

public:
  cxx11_constexpr string_view() : str(nullptr), len(-1) {}
  cxx11_constexpr string_view(const string_view &) = default;
  cxx14_constexpr string_view &
  operator=(const string_view &) cxx11_noexcept = default;

  cxx11_constexpr string_view(const char *str, std::size_t count)
      : str(str), len(str ? static_cast<intptr_t>(count) : -1) {
    constexpr_assert(len >= 0 || (len == -1 && !str));
  }

  cxx11_constexpr string_view(const char *begin, const char *end)
      : str(begin), len(begin ? static_cast<intptr_t>(end - begin) : -1) {
    constexpr_assert(end >= begin);
    constexpr_assert(len >= 0 || (len == -1 && !begin));
  }

  cxx11_constexpr string_view(const char *ptr)
      : str(ptr), len(ptr ? static_cast<intptr_t>(strlen(ptr)) : -1) {
    constexpr_assert(len >= 0 || (len == -1 && !str));
  }
  /* Конструктор из std::string ОБЯЗАН быть explicit для предотвращения
   * проблемы reference to temporary object из-за неявного создания string_view
   * из переданного по значению временного экземпляра std::string. */
  explicit /* не может быть cxx11_constexpr из-за std::string::size() */
      string_view(const std::string &s)
      : str(s.data()), len(static_cast<intptr_t>(s.size())) {
    assert(s.size() < npos);
  }
  operator std::string() const { return std::string(data(), length()); }

#if HAVE_std_string_view
  /* Конструктор из std::string_view:
   *  - Может быть НЕ-explicit, так как у std::string_view нет неявного
   *    конструктора из std::string. Поэтому не возникает проблемы
   *    reference to temporary object из-за неявного создания string_view
   *    из переданного по значению временного экземпляра std::string.
   *  - НЕ ДОЛЖЕН быть explicit для бесшовной интеграции с std::string_view. */
  cxx11_constexpr string_view(const std::string_view &v) cxx11_noexcept
      : str(v.data()),
        len(static_cast<intptr_t>(v.size())) {
    assert(v.size() < npos);
  }
  cxx11_constexpr operator std::string_view() const cxx11_noexcept {
    return std::string_view(data(), length());
  }
  cxx11_constexpr string_view &operator=(std::string_view &v) cxx11_noexcept {
    assert(v.size() < npos);
    this->str = v.data();
    this->len = static_cast<intptr_t>(v.size());
    return *this;
  }
  cxx11_constexpr void swap(std::string_view &v) cxx11_noexcept {
    const auto temp = *this;
    *this = v;
    v = temp;
  }
#endif /* HAVE_std_string_view */

  cxx14_constexpr void swap(string_view &v) cxx11_noexcept {
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
  cxx11_constexpr const_reference operator[](size_type pos) const {
    return str[pos];
  }
  cxx11_constexpr const_reference front() const {
    constexpr_assert(len > 0);
    return str[0];
  }
  cxx11_constexpr const_reference back() const {
    constexpr_assert(len > 0);
    return str[len - 1];
  }
  const_reference at(size_type pos) const;
  static cxx11_constexpr_var size_type npos = size_type(INT_MAX);

  typedef const char *const_iterator;
  cxx11_constexpr const_iterator cbegin() const { return str; }
  cxx11_constexpr const_iterator cend() const { return str + length(); }
  typedef const_iterator iterator;
  cxx11_constexpr iterator begin() const { return cbegin(); }
  cxx11_constexpr iterator end() const { return cend(); }

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

  cxx11_constexpr const char *data() const { return str; }
  cxx11_constexpr std::size_t length() const {
    return (len >= 0) ? (size_t)len : 0u;
  }
  cxx11_constexpr bool empty() const { return len <= 0; }
  cxx11_constexpr bool nil() const { return len < 0; }
  cxx11_constexpr std::size_t size() const { return length(); }
  cxx11_constexpr size_type max_size() const { return 32767; }

  cxx14_constexpr size_t hash_value() const {
    /* TODO: replace by t1ha_v3 */
    size_t h = static_cast<std::size_t>(len) * 3977471;
    for (intptr_t i = 0; i < len; ++i)
      h = (h ^ str[i]) * 1664525 + 1013904223;
    return h ^ 3863194411 * (h >> 11);
  }

  static cxx14_constexpr intptr_t compare(const string_view &a,
                                          const string_view &b) {
    const intptr_t diff = a.len - b.len;
    return diff               ? diff
           : (a.str == b.str) ? 0
                              : memcmp(a.data(), b.data(), a.length());
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

  static /* не может быть cxx11_constexpr из-за std::string::size() */ intptr_t
  compare(const std::string &a, const string_view &b) {
    return compare(string_view(a), b);
  }
  static /* не может быть cxx11_constexpr из-за std::string::size() */ intptr_t
  compare(const string_view &a, const std::string &b) {
    return compare(a, string_view(b));
  }
  bool operator==(const std::string &s) const { return compare(*this, s) == 0; }
  bool operator<(const std::string &s) const { return compare(*this, s) < 0; }
  bool operator>(const std::string &s) const { return compare(*this, s) > 0; }
  bool operator<=(const std::string &s) const { return compare(*this, s) <= 0; }
  bool operator>=(const std::string &s) const { return compare(*this, s) >= 0; }
  bool operator!=(const std::string &s) const { return compare(*this, s) != 0; }

  friend std::ostream &operator<<(std::ostream &out,
                                  const fptu::string_view &sv) {
    return out.write(sv.data(), sv.size());
  }
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
