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

#include "fast_positive/tuples_internal.h"
#include "fast_positive/tuples_legacy.h"

inline constexpr fptu_type genus2legacy(fptu::genus type, unsigned colnum = 0) {
  return fptu_type(fptu::details::make_tag(type, colnum, true, true, false));
}

struct FPTU_API_TYPE fptu_field : public fptu::details::field_loose {
  using base = fptu::details::field_loose;
  fptu_field() = delete;
  ~fptu_field() = delete;

  unsigned constexpr colnum() const { return base::id(); }
  fptu_type constexpr legacy_type() const { return genus2legacy(base::type()); }
};

struct FPTU_API_TYPE fptu_rw : public fptu::details::tuple_rw {
#ifdef FRIEND_TEST
  FRIEND_TEST(Fetch, Base);
  FRIEND_TEST(Fetch, Variate);
  FRIEND_TEST(Remove, Base);
#endif
  using base = fptu::details::tuple_rw;
  const fptu_field *begin_index() const noexcept {
    const auto begin = base::begin_index();
    return static_cast<const fptu_field *>(begin);
  }
  const fptu_field *end_index() const noexcept {
    const auto end = base::end_index();
    return static_cast<const fptu_field *>(end);
  }
};

using last_error = std::pair<fptu_error, std::string>;

const last_error &fptu_set_error(fptu_error code, const char *message);
const last_error &fptu_set_error(const std::exception &e);

struct error_guard {
  int *ptr;

  error_guard(int *error) : ptr(error) {}
  ~error_guard() {
    if (ptr)
      *ptr = FPTU_OK;
  }

  void feed(fptu_error code, const char *message) {
    fptu_set_error(code, message);
    if (ptr) {
      *ptr = code;
      ptr = nullptr;
    }
  }

  void feed(const std::exception &e) {
    fptu_error code = fptu_set_error(e).first;
    if (ptr) {
      *ptr = code;
      ptr = nullptr;
    }
  }
};

//------------------------------------------------------------------------------

#define FPTU_DENIL_FP32_BIN UINT32_C(0xFFFFffff)
static __inline float fptu_fp32_denil(void) {
  static const uint32_t u32 = FPTU_DENIL_FP32_BIN;
  return *erthink::constexpr_pointer_cast<const float *>(&u32);
}
#define FPTU_DENIL_FP32 fptu_fp32_denil()

#define FPTU_DENIL_FP64_BIN UINT64_C(0xFFFFffffFFFFffff)
static __inline double fptu_fp64_denil(void) {
  static const uint64_t u64 = FPTU_DENIL_FP64_BIN;
  return *erthink::constexpr_pointer_cast<const double *>(&u64);
}
#define FPTU_DENIL_FP64 fptu_fp64_denil()

#define FPTU_DENIL_CSTR nullptr
#define FPTU_DENIL_FIXBIN nullptr
#define FPTU_DENIL_DATETIME fptu::datetime_t::from_fixedpoint_32dot32(0)

//------------------------------------------------------------------------------

namespace fptu {

inline unsigned get_colnum(fptu_tag_t tag) {
  return fptu::details::tag2id(fptu::details::tag_t(tag));
}

inline fptu_type get_type(fptu_tag_t tag) {
  const fptu::genus type = fptu::details::tag2genus(tag);
  return genus2legacy(type);
}

inline bool tag_is_fixedsize(fptu_tag_t tag) {
  return fptu::details::is_fixed_size(tag);
}

inline bool tag_is_dead(fptu_tag_t tag) {
  return fptu::details::tag2genus(tag) == fptu::genus::hole;
}

inline fptu_tag_t make_tag(unsigned column, fptu_type type) {
  return genus2legacy(fptu::details::tag2genus(type), column);
}

template <typename type>
static __inline fptu_lge cmp2lge(type left, type right) {
  if (left == right)
    return fptu_eq;
  return (left < right) ? fptu_lt : fptu_gt;
}

template <typename type> static __inline fptu_lge diff2lge(type diff) {
  return cmp2lge<type>(diff, 0);
}

fptu_lge cmp2lge(fptu_lge left, fptu_lge right) = delete;
fptu_lge diff2lge(fptu_lge diff) = delete;

static __inline fptu_lge cmp_binary_str(const void *left_data,
                                        std::size_t left_len,
                                        const char *right_cstr) {
  std::size_t right_len = right_cstr ? strlen(right_cstr) : 0;
  return fptu_cmp_binary(left_data, left_len, right_cstr, right_len);
}

static __inline fptu_lge cmp_str_binary(const char *left_cstr,
                                        const void *right_data,
                                        std::size_t right_len) {
  std::size_t left_len = left_cstr ? strlen(left_cstr) : 0;
  return fptu_cmp_binary(left_cstr, left_len, right_data, right_len);
}

template <typename type> static __inline int cmp2int(type left, type right) {
  return (right > left) ? -1 : left > right;
}

template <typename iterator>
static __inline fptu_lge
depleted2lge(const iterator &left_pos, const iterator &left_end,
             const iterator &right_pos, const iterator &right_end) {
  bool left_depleted = (left_pos >= left_end);
  bool right_depleted = (right_pos >= right_end);

  if (left_depleted == right_depleted)
    return fptu_eq;
  return left_depleted ? fptu_lt : fptu_gt;
}

static inline __maybe_unused fptu_lge cmpbin(const void *a, const void *b,
                                             std::size_t bytes) {
  return diff2lge(std::memcmp(a, b, bytes));
}

} // namespace fptu
