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
#include "fast_positive/details/audit.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/exceptions.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/getter.h"
#include "fast_positive/details/meta.h"
#include "fast_positive/details/scan.h"
#include "fast_positive/details/schema.h"
#include "fast_positive/details/token.h"

namespace fptu {
namespace details {

class FPTU_API tuple_ro : private stretchy_value_tuple,
                          public crtp_getter<tuple_ro> {
  friend class crtp_getter<tuple_ro>;
  template <typename> friend class crtp_setter;
  friend class tuple_rw;

  tuple_ro() = delete;
  ~tuple_ro() = delete;
  tuple_ro &operator=(const tuple_ro &) = delete;

public:
  static const char *audit(const void *ptr, std::size_t bytes,
                           const schema *schema, audit_holes_info &);
  static const char *audit(const void *ptr, std::size_t bytes,
                           const schema *schema, bool holes_are_not_allowed) {
    audit_holes_info holes_info;
    const char *trouble = audit(ptr, bytes, schema, holes_info);
    if (unlikely(trouble))
      return trouble;
    if (holes_are_not_allowed) {
      if (unlikely(holes_info.holes_count))
        return "tuple have holes";
      assert(holes_info.holes_volume == 0);
    }
    return nullptr;
  }
  static const char *audit(const tuple_ro *self, const schema *schema,
                           bool holes_are_not_allowed) {
    return audit(self, self ? self->size() : 0, schema, holes_are_not_allowed);
  }
  static const tuple_ro *make_from_buffer(const void *ptr, std::size_t bytes,
                                          const schema *schema,
                                          bool skip_validation);

  const char *audit(const schema *schema, bool holes_are_not_allowed) const {
    return audit(this, schema, holes_are_not_allowed);
  }
  constexpr bool empty() const noexcept {
    return stretchy_value_tuple::brutto_units < 2;
  }
  constexpr std::size_t size() const noexcept {
    return stretchy_value_tuple::length();
  }
  constexpr const void *data() const noexcept { return this; }
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
