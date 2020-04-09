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

const char *fptu_check_ro_ex(fptu_ro ro,
                             bool holes_are_not_allowed) cxx11_noexcept {
  return fptu::details::tuple_ro::audit(ro.sys.iov_base, ro.sys.iov_len,
                                        nullptr, holes_are_not_allowed);
}

static cxx11_constexpr bool is_hollow(const fptu_ro &ro) cxx11_noexcept {
  return ro.sys.iov_len < sizeof(fptu::details::unit_t) ||
         static_cast<const fptu::details::tuple_ro *>(ro.sys.iov_base)
                 ->size() != ro.sys.iov_len;
}

bool fptu_is_empty_ro(fptu_ro ro) cxx11_noexcept {
  return is_hollow(ro)
             ? true
             : static_cast<const fptu::details::tuple_ro *>(ro.sys.iov_base)
                       ->index_size() < 1;
}

static inline const fptu::details::tuple_ro *impl(const fptu_ro &ro) {
  if (unlikely(is_hollow(ro)))
    fptu::throw_tuple_hollow();
  return static_cast<const fptu::details::tuple_ro *>(ro.sys.iov_base);
}

const fptu_field *fptu_begin_ro(fptu_ro ro) cxx11_noexcept {
  if (unlikely(is_hollow(ro)))
    return nullptr;

  const auto tuple =
      static_cast<const fptu::details::stretchy_value_tuple *>(ro.sys.iov_base);
  return static_cast<const fptu_field *>(tuple->begin_index());
}

const fptu_field *fptu_end_ro(fptu_ro ro) cxx11_noexcept {
  if (unlikely(is_hollow(ro)))
    return nullptr;

  const auto tuple =
      static_cast<const fptu::details::stretchy_value_tuple *>(ro.sys.iov_base);
  return static_cast<const fptu_field *>(tuple->end_index());
}

const fptu_field *
fptu_lookup_ro(fptu_ro ro, unsigned column,
               fptu_type_or_filter type_or_filter) cxx11_noexcept {
  const auto begin = fptu_begin_ro(ro);
  const auto end = fptu_end_ro(ro);
  const auto pf =
      const_cast<fptu_field *>(fptu_first(begin, end, column, type_or_filter));
  return (pf != end) ? pf : nullptr;
}

//------------------------------------------------------------------------------

#define FPTU_GET_IMPL(LEGACY, NAME, GENUS, RETURN_TYPE, THUNK_TYPE, DENIL)     \
  RETURN_TYPE fptu_get_##LEGACY(fptu_ro ro, unsigned column, int *error)       \
      cxx11_noexcept {                                                         \
    error_guard raii(error);                                                   \
    try {                                                                      \
      const fptu::token id(fptu::genus::GENUS, column, false);                 \
      return THUNK_TYPE(impl(ro)->get_##NAME(id));                             \
    } catch (const std::exception &e) {                                        \
      raii.feed(e);                                                            \
      return DENIL;                                                            \
    }                                                                          \
  }

FPTU_GET_IMPL(uint16, u16, u16, uint_fast16_t, uint_fast16_t, 0)
FPTU_GET_IMPL(bool, bool, boolean, bool, bool, false)
FPTU_GET_IMPL(int32, i32, i32, int_fast32_t, int_fast32_t, 0)
FPTU_GET_IMPL(uint32, u32, u32, uint_fast32_t, uint_fast32_t, 0)
FPTU_GET_IMPL(int64, i64, i64, int_fast64_t, int_fast64_t, 0)
FPTU_GET_IMPL(uint64, u64, u64, uint_fast64_t, uint_fast64_t, 0)
FPTU_GET_IMPL(fp64, f64, f64, double_t, double_t, FPTU_DENIL_FP64)
FPTU_GET_IMPL(fp32, f32, f32, float_t, float_t, FPTU_DENIL_FP32)
FPTU_GET_IMPL(datetime, datetime, t64, fptu_datetime_C, fptu_datetime_C,
              FPTU_DENIL_DATETIME)

FPTU_GET_IMPL(opaque, varbinary, varbin, ::iovec, fptu::details::iovec_thunk,
              fptu::details::iovec_thunk())
FPTU_GET_IMPL(nested, nested, nested, fptu_ro, fptu::details::iovec_thunk,
              fptu::details::iovec_thunk())
#undef FPTU_GET_IMPL

#define FPTU_GET_IMPL(BITS)                                                    \
  const uint8_t *fptu_get_##BITS(fptu_ro ro, unsigned column, int *error)      \
      cxx11_noexcept {                                                         \
    error_guard raii(error);                                                   \
    try {                                                                      \
      const fptu::token id(fptu::genus::bin##BITS, column, false);             \
      return erthink::constexpr_pointer_cast<const uint8_t *>(                 \
          &impl(ro)->get_bin##BITS(id));                                       \
    } catch (const std::exception &e) {                                        \
      raii.feed(e);                                                            \
      return nullptr;                                                          \
    }                                                                          \
  }

FPTU_GET_IMPL(96)
FPTU_GET_IMPL(128)
FPTU_GET_IMPL(160)
FPTU_GET_IMPL(256)
#undef FPTU_GET_IMPL

const char *fptu_get_cstr(fptu_ro ro, unsigned column,
                          int *error) cxx11_noexcept {
  const fptu_field *pf = fptu::lookup(ro, column, fptu_cstr);
  if (pf)
    return fptu_field_cstr(pf);
  fptu_set_error(FPTU_ENOFIELD, "fptu: no such field");
  if (error)
    *error = FPTU_ENOFIELD;
  return "";
}

//------------------------------------------------------------------------------

#define FPTU_GET_IMPL(BITS)                                                    \
  fptu_lge fptu_cmp_##BITS(fptu_ro ro, unsigned column, const uint8_t *value)  \
      cxx11_noexcept {                                                         \
    if (unlikely(value == nullptr))                                            \
      return fptu_ic;                                                          \
    const fptu_field *pf = fptu::lookup(ro, column, fptu_##BITS);              \
    if (unlikely(pf == nullptr))                                               \
      return fptu_ic;                                                          \
    return fptu::cmpbin(pf->relative.payload()->flat, value, BITS / 8);        \
  }

FPTU_GET_IMPL(96)
FPTU_GET_IMPL(128)
FPTU_GET_IMPL(160)
FPTU_GET_IMPL(256)
#undef FPTU_GET_IMPL

fptu_lge fptu_cmp_opaque(fptu_ro ro, unsigned column, const void *value,
                         std::size_t bytes) cxx11_noexcept {
  const fptu_field *pf = fptu::lookup(ro, column, fptu_opaque);
  if (pf == nullptr)
    return bytes ? fptu_ic : fptu_eq;

  const struct iovec iov = fptu_field_opaque(pf);
  return fptu_cmp_binary(iov.iov_base, iov.iov_len, value, bytes);
}

fptu_lge fptu_cmp_opaque_iov(fptu_ro ro, unsigned column,
                             const struct iovec value) cxx11_noexcept {
  return fptu_cmp_opaque(ro, column, value.iov_base, value.iov_len);
}
