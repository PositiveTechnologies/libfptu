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

#include "fast_positive/tuples/details/legacy_compat.h"
#include <typeinfo>

static thread_local last_error tls_last_error;

__cold void fptu_clear_error() {
  tls_last_error.first = FPTU_OK;
  tls_last_error.second.clear();
}

__cold fptu_error fptu_last_error_code() { return tls_last_error.first; }

__cold const char *fptu_last_error_msg() {
  return tls_last_error.second.c_str();
}

__cold const last_error &fptu_set_error(const std::exception &e) {
  auto &tls = tls_last_error;

  tls.second = e.what();
  if (typeid(e) == typeid(fptu::insufficient_space))
    tls.first = FPTU_ENOSPACE;
  else if (typeid(e) == typeid(std::bad_alloc))
    tls.first = FPTU_ENOMEM;
  else if (typeid(e) == typeid(fptu::field_absent))
    tls.first = FPTU_ENOFIELD;
  /* TODO ? */ else
    tls.first = FPTU_EINVAL;
  return tls;
}

__cold const last_error &fptu_set_error(fptu_error code, const char *message) {
  auto &tls = tls_last_error;
  tls.first = code;
  tls.second = message;
  return tls;
}

//------------------------------------------------------------------------------

static cxx11_constexpr bool is_filter(fptu_type_or_filter type_or_filter) {
  return (uint32_t(type_or_filter) & fptu_flag_not_filter) ? false : true;
}

static cxx11_constexpr fptu::details::tag_t
colid2tag(fptu_type_or_filter legacy_type, unsigned column) {
  return fptu::details::tag_t(legacy_type) +
         (column << fptu::details::tag_bits::id_shift);
}

static cxx11_constexpr bool match(const fptu_field *pf, unsigned column,
                                  fptu_type_or_filter type_or_filter) {
  CONSTEXPR_ASSERT(is_filter(type_or_filter));
  return !pf->is_hole() && pf->colnum() == column &&
         (fptu_filter(type_or_filter) & fptu_filter_mask(pf->legacy_type())) !=
             0;
}

__hot const fptu_field *
fptu_first(const fptu_field *begin, const fptu_field *end, unsigned column,
           fptu_type_or_filter type_or_filter) cxx11_noexcept {
  if (!is_filter(type_or_filter)) {
    const fptu_field *pf =
        static_cast<const fptu_field *>(fptu::details::lookup(
            false, begin, end, colid2tag(type_or_filter, column)));
    return pf ? pf : end;
  }

  // TODO: SIMD
  for (const fptu_field *pf = begin; pf < end; ++pf) {
    if (match(pf, column, type_or_filter))
      return pf;
  }
  return end;
}

__hot const fptu_field *
fptu_next(const fptu_field *from, const fptu_field *end, unsigned column,
          fptu_type_or_filter type_or_filter) cxx11_noexcept {
  return fptu_first(from + 1, end, column, type_or_filter);
}

__hot const fptu_field *fptu_first_ex(const fptu_field *begin,
                                      const fptu_field *end,
                                      fptu_field_filter filter, void *context,
                                      void *param) cxx11_noexcept {
  for (const fptu_field *pf = begin; pf < end; ++pf) {
    if (pf->is_hole())
      continue;
    if (filter(pf, context, param))
      return pf;
  }
  return end;
}

__hot const fptu_field *fptu_next_ex(const fptu_field *from,
                                     const fptu_field *end,
                                     fptu_field_filter filter, void *context,
                                     void *param) cxx11_noexcept {
  return fptu_first_ex(from + 1, end, filter, context, param);
}

//------------------------------------------------------------------------------

static std::size_t count(const fptu_field *begin, const fptu_field *end,
                         unsigned column, fptu_type_or_filter type_or_filter) {
  std::size_t result = 0;
  for (const fptu_field *pf = fptu_first(begin, end, column, type_or_filter);
       pf != end; pf = fptu_next(pf, end, column, type_or_filter))
    result++;

  return result;
}

static std::size_t count_ex(const fptu_field *begin, const fptu_field *end,
                            fptu_field_filter filter, void *context,
                            void *param) {
  std::size_t result = 0;
  for (const fptu_field *pf = fptu_first_ex(begin, end, filter, context, param);
       pf != end; pf = fptu_next_ex(pf, end, filter, context, param))
    result++;

  return result;
}

size_t fptu_field_count_rw(const fptu_rw *pt, unsigned column,
                           fptu_type_or_filter type_or_filter) cxx11_noexcept {
  return count(fptu_begin_rw(pt), fptu_end_rw(pt), column, type_or_filter);
}

size_t fptu_field_count_ro(fptu_ro ro, unsigned column,
                           fptu_type_or_filter type_or_filter) cxx11_noexcept {
  return count(fptu_begin_ro(ro), fptu_end_ro(ro), column, type_or_filter);
}

size_t fptu_field_count_rw_ex(const fptu_rw *pt, fptu_field_filter filter,
                              void *context, void *param) cxx11_noexcept {
  return count_ex(fptu_begin_rw(pt), fptu_end_rw(pt), filter, context, param);
}

size_t fptu_field_count_ro_ex(fptu_ro ro, fptu_field_filter filter,
                              void *context, void *param) cxx11_noexcept {
  return count_ex(fptu_begin_ro(ro), fptu_end_ro(ro), filter, context, param);
}

int fptu_erase(fptu_rw *pt, unsigned column,
               fptu_type_or_filter type_or_filter) cxx11_noexcept {
  if (unlikely(column > fptu_max_cols)) {
    static_assert(FPTU_EINVAL > 0, "should be positive");
    return -FPTU_EINVAL;
  }

  const auto begin = pt->begin_index();
  const auto end = pt->end_index();
  int result = 0;
  for (const fptu_field *pf = fptu_first(begin, end, column, type_or_filter);
       pf != end; pf = fptu_next(pf, end, column, type_or_filter)) {
    fptu_erase_field(pt, const_cast<fptu_field *>(pf));
    result++;
    if (!is_filter(type_or_filter))
      break;
  }

  return result;
}

//------------------------------------------------------------------------------

__cold const char *fptu_type_name(const fptu_type type) cxx11_noexcept {
  switch (fptu::details::tag_t(/* hush 'not in enumerated' */ type)) {
  default: {
    static __thread char buf[32];
    snprintf(buf, sizeof(buf), "invalid(fptu_type)%i", (int)type);
    return buf;
  }
  case fptu_uint16:
    return "uint16";
  case fptu_int32:
    return "int32";
  case fptu_uint32:
    return "uint32";
  case fptu_fp32:
    return "fp32";
  case fptu_int64:
    return "int64";
  case fptu_uint64:
    return "uint64";
  case fptu_fp64:
    return "fp64";
  case fptu_datetime:
    return "datetime";
  case fptu_96:
    return "b96";
  case fptu_128:
    return "b128";
  case fptu_160:
    return "b160";
  case fptu_256:
    return "b256";
  case fptu_cstr:
    return "cstr";
  case fptu_opaque:
    return "opaque";
  case fptu_nested:
    return "nested";
  }
}

//------------------------------------------------------------------------------
/* legacy C-compatible API */

unsigned fptu_get_colnum(fptu_tag_t tag) cxx11_noexcept {
  return fptu::get_colnum(tag);
}

fptu_type fptu_get_type(fptu_tag_t tag) cxx11_noexcept {
  return fptu::get_type(tag);
}

uint_fast16_t fptu_make_tag(unsigned column, fptu_type type) cxx11_noexcept {
  return fptu::make_tag(column, type);
}

bool fptu_tag_is_fixedsize(fptu_tag_t tag) cxx11_noexcept {
  return fptu::tag_is_fixedsize(tag);
}

bool fptu_tag_is_deleted(fptu_tag_t tag) cxx11_noexcept {
  return fptu::tag_is_dead(tag);
}

fptu_lge __hot fptu_cmp_binary(const void *left_data, std::size_t left_len,
                               const void *right_data,
                               std::size_t right_len) cxx11_noexcept {
  int diff = std::memcmp(left_data, right_data, std::min(left_len, right_len));
  if (diff == 0)
    diff = fptu::cmp2int(left_len, right_len);
  return fptu::diff2lge(diff);
}

const float fptu_fp32_denil_value =
    erthink::bit_cast<float>(uint32_t(FPTU_DENIL_FP32_BIN));
const double fptu_fp64_denil_value =
    erthink::bit_cast<double>(uint64_t(FPTU_DENIL_FP64_BIN));
