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
#include "fast_positive/tuples/details/audit.h"
#include "fast_positive/tuples/details/exceptions.h"
#include "fast_positive/tuples/details/field.h"
#include "fast_positive/tuples/details/getter.h"
#include "fast_positive/tuples/details/meta.h"
#include "fast_positive/tuples/details/scan.h"
#include "fast_positive/tuples/essentials.h"
#include "fast_positive/tuples/schema.h"
#include "fast_positive/tuples/token.h"

namespace fptu {
class tuple_ro_managed;
namespace details {

class FPTU_API_TYPE tuple_ro : private stretchy_value_tuple,
                               public crtp_getter<tuple_ro> {
  friend class crtp_getter<tuple_ro>;
  template <typename> friend class crtp_setter;
  friend class tuple_rw;
  friend class fptu::tuple_ro_managed /* mostly for assertions */;
  static inline const char *inline_lite_checkup(const void *ptr,
                                                std::size_t bytes) noexcept;

  tuple_ro() = delete;
  ~tuple_ro() = delete;
  tuple_ro &operator=(const tuple_ro &) = delete;

public:
  static const char *lite_checkup(const void *ptr, std::size_t bytes) noexcept;
  static const char *audit(const void *ptr, std::size_t bytes,
                           const fptu::schema *schema, audit_holes_info &);
  static const char *audit(const void *ptr, std::size_t bytes,
                           const fptu::schema *schema,
                           bool holes_are_not_allowed) {
    audit_holes_info holes_info;
    const char *trouble = audit(ptr, bytes, schema, holes_info);
    if (unlikely(trouble))
      return trouble;
    if (holes_are_not_allowed) {
      if (unlikely(holes_info.count))
        return "tuple have holes";
      assert(holes_info.volume == 0);
    }
    return nullptr;
  }
  static const char *audit(const tuple_ro *self, const fptu::schema *schema,
                           bool holes_are_not_allowed) {
    return audit(self, self ? self->size() : 0, schema, holes_are_not_allowed);
  }
  static const tuple_ro *make_from_buffer(const void *ptr, std::size_t bytes,
                                          const fptu::schema *schema,
                                          bool skip_validation);

  const char *audit(const fptu::schema *schema,
                    bool holes_are_not_allowed) const {
    return audit(this, schema, holes_are_not_allowed);
  }
  constexpr bool empty() const noexcept {
    return stretchy_value_tuple::brutto_units < 2;
  }
  constexpr std::size_t size() const noexcept {
    return stretchy_value_tuple::length();
  }
  constexpr const void *data() const noexcept { return this; }
  cxx14_constexpr std::size_t payload_size() const noexcept {
    return stretchy_value_tuple::payload_bytes();
  }
  constexpr const void *payload() const noexcept {
    return stretchy_value_tuple::begin_data_bytes();
  }
  constexpr std::size_t index_size() const noexcept {
    return stretchy_value_tuple::index_size();
  }
  constexpr bool is_sorted() const noexcept {
    return stretchy_value_tuple::is_sorted();
  }
  constexpr bool have_preplaced() const noexcept {
    return stretchy_value_tuple::have_preplaced();
  }
};

inline iovec_thunk::iovec_thunk(const tuple_ro *ro)
    : iovec(ro, ro ? ro->size() : 0) {}

} // namespace details
} // namespace fptu
