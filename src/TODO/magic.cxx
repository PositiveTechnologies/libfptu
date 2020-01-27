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

#if 0
#include "fast_positive/tuples/internal.h"

namespace fptu {

class magic_pointer : tagged_pointer_base {
  enum {
    KIND_BITS = fundamentals::kind_bitness,
    TYPE_BITS = fundamentals::typeid_bitness,
    IDENT_BITS = fundamentals::ident_bitness,

    KIND_MASK = (1u << KIND_BITS) - 1,
    TYPE_MASK = (1u << TYPE_BITS) - 1,
    IDENT_MASK = (1u << IDENT_BITS) - 1,

    KIND_SHIFT = 0,
    TYPE_SHIFT = KIND_SHIFT + KIND_BITS,
    IDENT_SHIFT = KIND_SHIFT + KIND_BITS,

    TAG_BITS = KIND_BITS + std::max(TYPE_BITS, IDENT_BITS),

    WITH_TYPE_BITSET =
        utils::bitset_mask<kind_pointer::extobj, kind_pointer::fixlen_value,
                           kind_pointer::varlen_value,
                           kind_pointer::varlen_pool, kind_pointer::field_rw,
                           kind_pointer::field_ro>::value
        << KIND_SHIFT,

    WITH_IDENT_BITSET = utils::bitset_mask<kind_pointer::incorporeal_rw,
                                           kind_pointer::schema_ident>::value
                        << KIND_SHIFT,

    WITH_VALUE_BITSET =
        utils::bitset_mask<kind_pointer::extobj, kind_pointer::fixlen_value,
                           kind_pointer::varlen_value,
                           kind_pointer::varlen_pool, kind_pointer::field_rw,
                           kind_pointer::field_ro>::value
        << KIND_SHIFT,

    WITH_WRITABLE_BITSET =
        utils::bitset_mask<kind_pointer::extobj, kind_pointer::fixlen_value,
                           kind_pointer::varlen_value,
                           kind_pointer::field_rw>::value
        << KIND_SHIFT,

    WITH_TUPLE_BITSET = utils::bitset_mask<kind_pointer::field_rw,
                                           kind_pointer::field_ro>::value
                        << KIND_SHIFT,
  };

protected:
  static constexpr unsigned make_pointer_tag(kind_pointer kind,
                                             value_genus type) {
    static_assert(TAG_BITS <= tagged_pointer_base::bits, "WTF?");
    constexpr_assert(kind <= unsigned(KIND_MASK));
    constexpr_assert(type <= unsigned(TYPE_MASK));
    return unsigned(kind) << KIND_SHIFT | unsigned(type) << TYPE_SHIFT;
  }

  static constexpr unsigned make_pointer_tag(kind_pointer kind,
                                             unsigned field_id) {
    static_assert(TAG_BITS <= tagged_pointer_base::bits, "WTF?");
    constexpr_assert(kind <= unsigned(KIND_MASK));
    constexpr_assert(field_id <= unsigned(IDENT_MASK));
    return unsigned(kind) << KIND_SHIFT | field_id << IDENT_SHIFT;
  }

  value_genus builtin_type() const {
    assert(has_builtin_type());
    return value_genus((basis::tag() >> TYPE_SHIFT) & TYPE_MASK);
  }
  value_genus outsource_type() const;

  field_ident builtin_ident() const {
    assert(has_builtin_ident());
    return field_ident((basis::tag() >> IDENT_SHIFT) & IDENT_MASK);
  }

  field_ident outsource_ident() const;

public:
  using basis = tagged_pointer_base;
  kind_pointer kind() const {
    return kind_pointer((basis::tag() >> KIND_SHIFT) & KIND_MASK);
  }

  bool has_builtin_type() const {
    return (basis::tag() & WITH_TYPE_BITSET) != 0;
  }
  bool has_builtin_ident() const {
    return (basis::tag() & WITH_IDENT_BITSET) != 0;
  }

  value_genus type() const {
    return likely(has_builtin_type()) ? builtin_type() : outsource_type();
  }

  field_ident ident() const {
    return likely(has_builtin_ident()) ? builtin_ident() : outsource_ident();
  }

  //  constexpr tagged_pointer(T *ptr, field_pointer_kind kind,
  //                           value_genus type)
  //      : basis(ptr, tag) {}
  //  constexpr tagged_pointer() : basis() {}

  //  void set(T *ptr, unsigned tag) { basis::set(ptr, tag); }
  //  void set_ptr(void *ptr) { basis::set_ptr(ptr); }
  //  void set_tag(unsigned tag) { basis::set_tag(tag); }
  //  unsigned tag() const { return basis::tag(); }
  //  T *get() const { return reinterpret_cast<T *>(basis::ptr()); }
  //  T *operator->() const { return get(); }
  //  reference operator*() const { return *get(); }
  //  reference operator[](std::ptrdiff_t i) const { return get()[i]; }
};

// value_genus magic_pointer::outsource_type() const {
//  assert(!has_builtin_type());
//  switch (kind()) {
//  case kind_pointer::extobj:
//  case kind_pointer::incorporeal_rw:
//  case kind_pointer::schema_ident:
//    // TODO
//  default:
//    assert(false);
//    __unreachable();
//    return value_genus::nothing;
//  }
//}

// field_ident magic_pointer::outsource_ident() const {
//  assert(!has_builtin_ident());
//  switch (kind()) {
//  default:
//    assert(false);
//    __unreachable();
//  case kind_pointer::extobj:
//  case kind_pointer::fixlen_value:
//  case kind_pointer::varlen_value:
//    return field_ident::invalid;

//  case kind_pointer::incorporeal_rw:
//  case kind_pointer::schema_ident:
//    // TODO
//    return field_ident::invalid;
//  }
//}

} // namespace fptu

#endif
