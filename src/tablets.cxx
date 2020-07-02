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

#include "fast_positive/tuples/internal.h"

#include "fast_positive/erthink/erthink_arch.h"
#include "fast_positive/tuples/details/meta.h"
#include "fast_positive/tuples/essentials.h"

#include "fast_positive/tuples/details/warnings_push_pt.h"

namespace fptu {
namespace details {

alignas(64) const char zeroed_cacheline[64] = {0};

namespace {
struct tag_static_ensure {
  tag_static_ensure() {
    static_assert(loose_end == (tag_bits::inlay_flag >> tag_bits::id_shift),
                  "WTF?");
    static_assert(loose_end == (1u << fundamentals::ident_bitness) / 2, "WTF?");
    static_assert(inlay_end == (1u << fundamentals::ident_bitness), "WTF?");
  }
};
} // namespace

template <typename T> static cxx11_constexpr T thunk_31_hole(const T value) {
  static_assert(genus::hole == 31, "WTF?");
  return value;
}

#if !defined(_MSC_VER) || defined(__clang__)
cxx11_constexpr_var
#else
const
#endif
    uint8_t genus2looseunits_array[32] = {
        meta::genus_traits<genus(0)>::loose_units,
        meta::genus_traits<genus(1)>::loose_units,
        meta::genus_traits<genus(2)>::loose_units,
        meta::genus_traits<genus(3)>::loose_units,
        meta::genus_traits<genus(4)>::loose_units,
        meta::genus_traits<genus(5)>::loose_units,
        meta::genus_traits<genus(6)>::loose_units,
        meta::genus_traits<genus(7)>::loose_units,
        meta::genus_traits<genus(8)>::loose_units,
        meta::genus_traits<genus(9)>::loose_units,
        meta::genus_traits<genus(10)>::loose_units,
        meta::genus_traits<genus(11)>::loose_units,
        meta::genus_traits<genus(12)>::loose_units,
        meta::genus_traits<genus(13)>::loose_units,
        meta::genus_traits<genus(14)>::loose_units,
        meta::genus_traits<genus(15)>::loose_units,
        meta::genus_traits<genus(16)>::loose_units,
        meta::genus_traits<genus(17)>::loose_units,
        meta::genus_traits<genus(18)>::loose_units,
        meta::genus_traits<genus(19)>::loose_units,
        meta::genus_traits<genus(20)>::loose_units,
        meta::genus_traits<genus(21)>::loose_units,
        meta::genus_traits<genus(22)>::loose_units,
        meta::genus_traits<genus(23)>::loose_units,
        meta::genus_traits<genus(24)>::loose_units,
        meta::genus_traits<genus(25)>::loose_units,
        meta::genus_traits<genus(26)>::loose_units,
        meta::genus_traits<genus(27)>::loose_units,
        meta::genus_traits<genus(28)>::loose_units,
        meta::genus_traits<genus(29)>::loose_units,
        meta::genus_traits<genus(30)>::loose_units,
        thunk_31_hole<uint8_t>(0)};

#if !defined(_MSC_VER) || defined(__clang__)
cxx11_constexpr_var
#else
const
#endif
    uint8_t genus2preplacedbytes_array[32] = {
        meta::genus_traits<genus(0)>::preplaced_bytes,
        meta::genus_traits<genus(1)>::preplaced_bytes,
        meta::genus_traits<genus(2)>::preplaced_bytes,
        meta::genus_traits<genus(3)>::preplaced_bytes,
        meta::genus_traits<genus(4)>::preplaced_bytes,
        meta::genus_traits<genus(5)>::preplaced_bytes,
        meta::genus_traits<genus(6)>::preplaced_bytes,
        meta::genus_traits<genus(7)>::preplaced_bytes,
        meta::genus_traits<genus(8)>::preplaced_bytes,
        meta::genus_traits<genus(9)>::preplaced_bytes,
        meta::genus_traits<genus(10)>::preplaced_bytes,
        meta::genus_traits<genus(11)>::preplaced_bytes,
        meta::genus_traits<genus(12)>::preplaced_bytes,
        meta::genus_traits<genus(13)>::preplaced_bytes,
        meta::genus_traits<genus(14)>::preplaced_bytes,
        meta::genus_traits<genus(15)>::preplaced_bytes,
        meta::genus_traits<genus(16)>::preplaced_bytes,
        meta::genus_traits<genus(17)>::preplaced_bytes,
        meta::genus_traits<genus(18)>::preplaced_bytes,
        meta::genus_traits<genus(19)>::preplaced_bytes,
        meta::genus_traits<genus(20)>::preplaced_bytes,
        meta::genus_traits<genus(21)>::preplaced_bytes,
        meta::genus_traits<genus(22)>::preplaced_bytes,
        meta::genus_traits<genus(23)>::preplaced_bytes,
        meta::genus_traits<genus(24)>::preplaced_bytes,
        meta::genus_traits<genus(25)>::preplaced_bytes,
        meta::genus_traits<genus(26)>::preplaced_bytes,
        meta::genus_traits<genus(27)>::preplaced_bytes,
        meta::genus_traits<genus(28)>::preplaced_bytes,
        meta::genus_traits<genus(29)>::preplaced_bytes,
        meta::genus_traits<genus(30)>::preplaced_bytes,
        thunk_31_hole<uint8_t>(0)};

#if !defined(_MSC_VER) || defined(__clang__)
cxx11_constexpr_var
#else
const
#endif
    genus_mask_t trivially_convertible_from_array[32] = {
        meta::genus_traits<genus(0)>::trivially_convertible_from,
        meta::genus_traits<genus(1)>::trivially_convertible_from,
        meta::genus_traits<genus(2)>::trivially_convertible_from,
        meta::genus_traits<genus(3)>::trivially_convertible_from,
        meta::genus_traits<genus(4)>::trivially_convertible_from,
        meta::genus_traits<genus(5)>::trivially_convertible_from,
        meta::genus_traits<genus(6)>::trivially_convertible_from,
        meta::genus_traits<genus(7)>::trivially_convertible_from,
        meta::genus_traits<genus(8)>::trivially_convertible_from,
        meta::genus_traits<genus(9)>::trivially_convertible_from,
        meta::genus_traits<genus(10)>::trivially_convertible_from,
        meta::genus_traits<genus(11)>::trivially_convertible_from,
        meta::genus_traits<genus(12)>::trivially_convertible_from,
        meta::genus_traits<genus(13)>::trivially_convertible_from,
        meta::genus_traits<genus(14)>::trivially_convertible_from,
        meta::genus_traits<genus(15)>::trivially_convertible_from,
        meta::genus_traits<genus(16)>::trivially_convertible_from,
        meta::genus_traits<genus(17)>::trivially_convertible_from,
        meta::genus_traits<genus(18)>::trivially_convertible_from,
        meta::genus_traits<genus(19)>::trivially_convertible_from,
        meta::genus_traits<genus(20)>::trivially_convertible_from,
        meta::genus_traits<genus(21)>::trivially_convertible_from,
        meta::genus_traits<genus(22)>::trivially_convertible_from,
        meta::genus_traits<genus(23)>::trivially_convertible_from,
        meta::genus_traits<genus(24)>::trivially_convertible_from,
        meta::genus_traits<genus(25)>::trivially_convertible_from,
        meta::genus_traits<genus(26)>::trivially_convertible_from,
        meta::genus_traits<genus(27)>::trivially_convertible_from,
        meta::genus_traits<genus(28)>::trivially_convertible_from,
        meta::genus_traits<genus(29)>::trivially_convertible_from,
        meta::genus_traits<genus(30)>::trivially_convertible_from,
        thunk_31_hole<genus_mask_t>(utils::bitset_mask<hole>::value)};

bool field_preplaced::is_null(tag_t tag) const cxx11_noexcept {
  assert(is_preplaced(tag));
  switch (tag2genus(tag)) {
  default:
    __unreachable();
    assert(false);
#define HERE_CASE_ITEM(N)                                                      \
  case genus(N):                                                               \
    assert(tag2indysize(tag) ==                                                \
           meta::genus_traits<genus(N)>::preplaced_bytes);                     \
    return (!is_fixed_size(genus(N)) || is_discernible_null(tag)) &&           \
           meta::genus_traits<genus(N)>::is_denil(this)
    HERE_CASE_ITEM(0);
    HERE_CASE_ITEM(1);
    HERE_CASE_ITEM(2);
    HERE_CASE_ITEM(3);
    HERE_CASE_ITEM(4);
    HERE_CASE_ITEM(5);
    HERE_CASE_ITEM(6);
    HERE_CASE_ITEM(7);
    HERE_CASE_ITEM(8);
    HERE_CASE_ITEM(9);
    HERE_CASE_ITEM(10);
    HERE_CASE_ITEM(11);
    HERE_CASE_ITEM(12);
    HERE_CASE_ITEM(13);
    HERE_CASE_ITEM(14);
    HERE_CASE_ITEM(15);
    HERE_CASE_ITEM(16);
    HERE_CASE_ITEM(17);
    HERE_CASE_ITEM(18);
    HERE_CASE_ITEM(19);
    HERE_CASE_ITEM(20);
    HERE_CASE_ITEM(21);
    HERE_CASE_ITEM(22);
    HERE_CASE_ITEM(23);
    HERE_CASE_ITEM(24);
    HERE_CASE_ITEM(25);
    HERE_CASE_ITEM(26);
    HERE_CASE_ITEM(27);
    HERE_CASE_ITEM(28);
    HERE_CASE_ITEM(29);
    HERE_CASE_ITEM(30);
#undef HERE_CASE_ITEM
  case hole:
    static_assert(genus::hole == 31, "WTF?");
    return hippeus::is_zeroed(this, tag2indysize(tag));
  }
}

} // namespace details
} // namespace fptu
