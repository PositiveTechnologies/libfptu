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

#include "fast_positive/details/legacy_compat.h"

size_t fptu_space(size_t items, std::size_t data_bytes) noexcept {
  return fptu::details::tuple_rw::estimate_required_space(
      std::min(items, size_t(fptu::max_fields)),
      std::min(data_bytes, size_t(fptu::max_tuple_bytes_netto)), nullptr);
}

fptu_rw *fptu_init(void *buffer_space, std::size_t buffer_bytes,
                   std::size_t items_limit) noexcept {
  if (unlikely(buffer_space == nullptr))
    return nullptr;

  try {
    const auto tuple = fptu::details::tuple_rw::create_legacy(
        buffer_space, buffer_bytes, items_limit);
    return static_cast<fptu_rw *>(tuple);
  } catch (const std::exception &e) {
    fptu_set_error(e);
    return nullptr;
  }
}

fptu_rw *fptu_alloc(size_t items_limit, std::size_t data_bytes) noexcept {
  if (unlikely(items_limit > fptu::max_fields ||
               data_bytes > fptu::max_tuple_bytes_netto))
    return nullptr;

  const std::size_t bytes = fptu_space(items_limit, data_bytes);
  return fptu_init(::malloc(bytes), bytes, items_limit);
}

void fptu_destroy(fptu_rw *pt) noexcept {
  if (pt)
    pt->~fptu_rw();
}

fptu_error fptu_clear(fptu_rw *pt) noexcept {
  if (unlikely(pt == nullptr))
    return FPTU_EINVAL;

  try {
    pt->reset();
  } catch (const fptu::bad_tuple_rw &e) {
    return fptu_set_error(e).first;
  }
  return FPTU_OK;
}

size_t fptu_space4items(const fptu_rw *pt) noexcept { return pt->head_space(); }

size_t fptu_space4data(const fptu_rw *pt) noexcept {
  return pt->tail_space_bytes();
}

size_t fptu_junkspace(const fptu_rw *pt) noexcept { return pt->junk_bytes(); }

const char *fptu_check_rw(const fptu_rw *pt) noexcept { return pt->audit(); }

fptu_ro fptu_take_noshrink(const fptu_rw *pt) noexcept {
  const fptu::details::tuple_ro *ro = pt->take_asis();
  fptu_ro envelope;
  envelope.sys.iov_base = (void *)ro;
  envelope.sys.iov_len = ro->size();
  return envelope;
}

bool fptu_shrink(fptu_rw *pt) noexcept {
  try {
    return pt->optimize();
  } catch (const std::exception &e) {
    fptu_set_error(e);
    return true;
  }
}

size_t fptu_check_and_get_buffer_size(fptu_ro ro, unsigned more_items,
                                      unsigned more_payload,
                                      const char **error) noexcept {
  if (unlikely(error == nullptr))
    return ~size_t(0);

  *error = fptu_check_ro(ro);
  if (unlikely(*error != nullptr))
    return 0;

  const fptu::details::tuple_ro *tuple_ro =
      static_cast<const fptu::details::tuple_ro *>(ro.sys.iov_base);
  try {
    return fptu::details::tuple_rw::estimate_required_space(
        tuple_ro, more_items, more_payload, nullptr);
  } catch (const std::exception &e) {
    *error = fptu_set_error(e).second.c_str();
    return 0;
  }
}

fptu_rw *fptu_fetch(fptu_ro ro, void *buffer_space, std::size_t buffer_bytes,
                    unsigned more_items) noexcept {
  if (unlikely(buffer_space == nullptr)) {
    fptu_set_error(FPTU_EINVAL, "invalid buffer (NULL)");
    return nullptr;
  }

  fptu::details::unit_t stub_empty;
  if (unlikely(ro.sys.iov_len == 0)) {
    static_assert(sizeof(stub_empty) == sizeof(fptu::details::tuple_ro),
                  "Oops");
    auto *stub =
        reinterpret_cast<fptu::details::stretchy_value_tuple *>(&stub_empty);
    stub->brutto_units = 1;
    stub->looseitems_flags = 0;
    ro.sys.iov_base = &stub_empty;
    ro.sys.iov_len = sizeof(stub_empty);
  }

  const char *trouble =
      fptu::details::tuple_ro::lite_checkup(ro.sys.iov_base, ro.sys.iov_len);
  if (unlikely(trouble != nullptr)) {
    fptu_set_error(FPTU_EINVAL, trouble);
    return nullptr;
  }

  try {
    const fptu::details::tuple_ro *tuple_ro =
        static_cast<const fptu::details::tuple_ro *>(ro.sys.iov_base);

    const auto tuple_rw = fptu::details::tuple_rw::fetch_legacy(
        {0, 0}, tuple_ro, buffer_space, buffer_bytes, more_items);
    return static_cast<fptu_rw *>(tuple_rw);
  } catch (const std::exception &e) {
    fptu_set_error(e);
    return nullptr;
  }
}

size_t fptu_get_buffer_size(fptu_ro ro, unsigned more_items,
                            unsigned more_payload) noexcept {
  const fptu::details::tuple_ro *tuple_ro =
      static_cast<const fptu::details::tuple_ro *>(ro.sys.iov_base);
  return fptu::details::tuple_rw::estimate_required_space(
      tuple_ro, more_items, more_payload, nullptr);
}

static const char cbfs_ok_sign[] = "FPTU_SUCCESS";

fptu_rw *fptu_fetch_ex(fptu_ro ro, void *buffer_space, std::size_t buffer_bytes,
                       unsigned more_items,
                       struct fptu_cbfs_result *cbfs) noexcept {

  if (unlikely(cbfs == nullptr || cbfs->err || cbfs->err_msg != cbfs_ok_sign)) {
    fptu_set_error(FPTU_EINVAL, "invalid cbfs");
    return nullptr;
  }
  if (unlikely(buffer_space == nullptr)) {
    fptu_set_error(FPTU_EINVAL, "invalid buffer (NULL)");
    return nullptr;
  }

  fptu::details::audit_holes_info holes_info;
  holes_info.count = cbfs->holes_count;
  holes_info.volume = cbfs->holes_volume;
  const fptu::details::tuple_ro *tuple_ro =
      static_cast<const fptu::details::tuple_ro *>(ro.sys.iov_base);
  try {
    const auto tuple_rw = fptu::details::tuple_rw::fetch_legacy(
        holes_info, tuple_ro, buffer_space, buffer_bytes, more_items);
    return static_cast<fptu_rw *>(tuple_rw);
  } catch (const std::exception &e) {
    fptu_set_error(e);
    return nullptr;
  }
}

size_t
fptu_check_and_get_buffer_size_ex(fptu_ro ro, unsigned more_items,
                                  unsigned more_payload,
                                  struct fptu_cbfs_result *cbfs) noexcept {
  if (unlikely(cbfs == nullptr))
    return ~size_t(0);

  fptu::details::audit_holes_info holes_info;
  const char *trouble = fptu::details::tuple_ro::audit(
      ro.sys.iov_base, ro.sys.iov_len, nullptr, holes_info);
  if (unlikely(trouble)) {
    const auto &error_info = fptu_set_error(FPTU_EINVAL, trouble);
    cbfs->err = error_info.first;
    cbfs->err_msg = error_info.second.c_str();
    return 0;
  }

  const fptu::details::tuple_ro *tuple_ro =
      static_cast<const fptu::details::tuple_ro *>(ro.sys.iov_base);
  try {
    const auto result = fptu::details::tuple_rw::estimate_required_space(
        tuple_ro, more_items, more_payload, nullptr);
    cbfs->err = FPTU_OK;
    cbfs->err_msg = cbfs_ok_sign;
    cbfs->holes_count = uint16_t(holes_info.count);
    cbfs->holes_volume = uint16_t(holes_info.volume);
    return result;
  } catch (const std::exception &e) {
    const auto &error_info = fptu_set_error(e);
    cbfs->err = error_info.first;
    cbfs->err_msg = error_info.second.c_str();
    return 0;
  }
}

const fptu_field *fptu_begin_rw(const fptu_rw *pt) noexcept {
  return pt->begin_index();
}
const fptu_field *fptu_end_rw(const fptu_rw *pt) noexcept {
  return pt->end_index();
}
bool fptu_is_empty_rw(const fptu_rw *pt) noexcept { return pt->empty(); }

fptu_field *fptu_lookup_rw(fptu_rw *pt, unsigned column,
                           fptu_type_or_filter type_or_filter) noexcept {
  const auto begin = pt->begin_index();
  const auto end = pt->end_index();
  const auto pf =
      const_cast<fptu_field *>(fptu_first(begin, end, column, type_or_filter));
  return (pf != end) ? pf : nullptr;
}

//------------------------------------------------------------------------------

#define FPTU_UPSERT_IMPL(LEGACY, NAME, GENUS, VALUE_TYPE)                      \
  fptu_error fptu_upsert_##LEGACY(fptu_rw *pt, unsigned column,                \
                                  VALUE_TYPE value) noexcept {                 \
    try {                                                                      \
      const fptu::token id(fptu::genus::GENUS, column, false);                 \
      pt->set_##NAME(id, value);                                               \
      return FPTU_OK;                                                          \
    } catch (const std::exception &e) {                                        \
      return fptu_set_error(e).first;                                          \
    }                                                                          \
  }

FPTU_UPSERT_IMPL(uint16, u16, u16, uint16_t)
FPTU_UPSERT_IMPL(bool, bool, boolean, bool)
FPTU_UPSERT_IMPL(int32, i32, i32, int_fast32_t)
FPTU_UPSERT_IMPL(uint32, u32, u32, uint_fast32_t)
FPTU_UPSERT_IMPL(int64, i64, i64, int_fast64_t)
FPTU_UPSERT_IMPL(uint64, u64, u64, uint_fast64_t)
FPTU_UPSERT_IMPL(fp64, f64, f64, double_t)
FPTU_UPSERT_IMPL(fp32, f32, f32, float_t)
FPTU_UPSERT_IMPL(datetime, datetime, t64, fptu_datetime_t)
#undef FPTU_UPSERT_IMPL

fptu_error fptu_upsert_null(fptu_rw *pt, unsigned column) noexcept {
  try {
    const fptu::token id(fptu::genus::enumeration, column, false);
    pt->set_enum(
        id, fptu::meta::genus_traits<fptu::genus::enumeration>::optional_denil);
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}

#define FPTU_GET_IMPL(BITS)                                                    \
  fptu_error fptu_upsert_##BITS(fptu_rw *pt, unsigned column,                  \
                                const void *data) noexcept {                   \
    try {                                                                      \
      const fptu::token id(fptu::genus::bin##BITS, column, false);             \
      pt->set_bin##BITS(                                                       \
          id,                                                                  \
          *erthink::constexpr_pointer_cast<const fptu::binary##BITS##_t *>(    \
              data));                                                          \
      return FPTU_OK;                                                          \
    } catch (const std::exception &e) {                                        \
      return fptu_set_error(e).first;                                          \
    }                                                                          \
  }

FPTU_GET_IMPL(96)
FPTU_GET_IMPL(128)
FPTU_GET_IMPL(160)
FPTU_GET_IMPL(256)
#undef FPTU_GET_IMPL

fptu_error fptu_upsert_string(fptu_rw *pt, unsigned column, const char *text,
                              std::size_t length) noexcept {
  try {
    const fptu::token id(fptu::genus::text, column, false);
    pt->set_string(id, fptu::string_view(text, length));
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}

fptu_error fptu_upsert_opaque(fptu_rw *pt, unsigned column, const void *data,
                              std::size_t length) noexcept {
  try {
    const fptu::token id(fptu::genus::varbin, column, false);
    pt->set_varbinary(
        id, fptu::string_view(static_cast<const char *>(data), length));
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}

fptu_error fptu_upsert_opaque_iov(fptu_rw *pt, unsigned column,
                                  const struct iovec value) noexcept {
  return fptu_upsert_opaque(pt, column, value.iov_base, value.iov_len);
}

fptu_error fptu_upsert_nested(fptu_rw *pt, unsigned column,
                              fptu_ro ro) noexcept {
  try {
    const fptu::token id(fptu::genus::varbin, column, false);
    pt->set_nested(id, fptu::details::tuple_ro::make_from_buffer(
                           ro.sys.iov_base, ro.sys.iov_len, nullptr, false));
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}

//------------------------------------------------------------------------------

#define FPTU_INSERT_IMPL(LEGACY, NAME, GENUS, VALUE_TYPE)                      \
  fptu_error fptu_insert_##LEGACY(fptu_rw *pt, unsigned column,                \
                                  VALUE_TYPE value) noexcept {                 \
    try {                                                                      \
      const fptu::token id(fptu::genus::GENUS, column, true);                  \
      pt->insert_##NAME(id, value);                                            \
      return FPTU_OK;                                                          \
    } catch (const std::exception &e) {                                        \
      return fptu_set_error(e).first;                                          \
    }                                                                          \
  }

FPTU_INSERT_IMPL(uint16, u16, u16, uint16_t)
FPTU_INSERT_IMPL(bool, bool, boolean, bool)
FPTU_INSERT_IMPL(int32, i32, i32, int_fast32_t)
FPTU_INSERT_IMPL(uint32, u32, u32, uint_fast32_t)
FPTU_INSERT_IMPL(int64, i64, i64, int_fast64_t)
FPTU_INSERT_IMPL(uint64, u64, u64, uint_fast64_t)
FPTU_INSERT_IMPL(fp64, f64, f64, double_t)
FPTU_INSERT_IMPL(fp32, f32, f32, float_t)
FPTU_INSERT_IMPL(datetime, datetime, t64, fptu_datetime_t)
#undef FPTU_INSERT_IMPL

#define FPTU_GET_IMPL(BITS)                                                    \
  fptu_error fptu_insert_##BITS(fptu_rw *pt, unsigned column,                  \
                                const void *data) noexcept {                   \
    try {                                                                      \
      const fptu::token id(fptu::genus::bin##BITS, column, true);              \
      pt->insert_bin##BITS(                                                    \
          id,                                                                  \
          *erthink::constexpr_pointer_cast<const fptu::binary##BITS##_t *>(    \
              data));                                                          \
      return FPTU_OK;                                                          \
    } catch (const std::exception &e) {                                        \
      return fptu_set_error(e).first;                                          \
    }                                                                          \
  }

FPTU_GET_IMPL(96)
FPTU_GET_IMPL(128)
FPTU_GET_IMPL(160)
FPTU_GET_IMPL(256)
#undef FPTU_GET_IMPL

fptu_error fptu_insert_string(fptu_rw *pt, unsigned column, const char *text,
                              std::size_t length) noexcept {
  try {
    const fptu::token id(fptu::genus::text, column, true);
    pt->insert_string(id, fptu::string_view(text, length));
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}

fptu_error fptu_insert_opaque(fptu_rw *pt, unsigned column, const void *data,
                              std::size_t length) noexcept {
  try {
    const fptu::token id(fptu::genus::varbin, column, true);
    pt->insert_varbinary(
        id, fptu::string_view(static_cast<const char *>(data), length));
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}

fptu_error fptu_insert_opaque_iov(fptu_rw *pt, unsigned column,
                                  const struct iovec value) noexcept {
  return fptu_insert_opaque(pt, column, value.iov_base, value.iov_len);
}

fptu_error fptu_insert_nested(fptu_rw *pt, unsigned column,
                              fptu_ro ro) noexcept {
  try {
    const fptu::token id(fptu::genus::nested, column, true);
    pt->insert_nested(id, fptu::details::tuple_ro::make_from_buffer(
                              ro.sys.iov_base, ro.sys.iov_len, nullptr, false));
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}

//------------------------------------------------------------------------------

#define FPTU_UPDATE_IMPL(LEGACY, NAME, GENUS, VALUE_TYPE)                      \
  fptu_error fptu_update_##LEGACY(fptu_rw *pt, unsigned column,                \
                                  VALUE_TYPE value) noexcept {                 \
    try {                                                                      \
      const fptu::token id(fptu::genus::GENUS, column, true);                  \
      pt->legacy_update_##NAME(id, value);                                     \
      return FPTU_OK;                                                          \
    } catch (const std::exception &e) {                                        \
      return fptu_set_error(e).first;                                          \
    }                                                                          \
  }

FPTU_UPDATE_IMPL(uint16, u16, u16, uint16_t)
FPTU_UPDATE_IMPL(bool, bool, boolean, bool)
FPTU_UPDATE_IMPL(int32, i32, i32, int_fast32_t)
FPTU_UPDATE_IMPL(uint32, u32, u32, uint_fast32_t)
FPTU_UPDATE_IMPL(int64, i64, i64, int_fast64_t)
FPTU_UPDATE_IMPL(uint64, u64, u64, uint_fast64_t)
FPTU_UPDATE_IMPL(fp64, f64, f64, double_t)
FPTU_UPDATE_IMPL(fp32, f32, f32, float_t)
FPTU_UPDATE_IMPL(datetime, datetime, t64, fptu_datetime_t)
#undef FPTU_UPDATE_IMPL

#define FPTU_GET_IMPL(BITS)                                                    \
  fptu_error fptu_update_##BITS(fptu_rw *pt, unsigned column,                  \
                                const void *data) noexcept {                   \
    try {                                                                      \
      const fptu::token id(fptu::genus::bin##BITS, column, true);              \
      pt->legacy_update_bin##BITS(                                             \
          id,                                                                  \
          *erthink::constexpr_pointer_cast<const fptu::binary##BITS##_t *>(    \
              data));                                                          \
      return FPTU_OK;                                                          \
    } catch (const std::exception &e) {                                        \
      return fptu_set_error(e).first;                                          \
    }                                                                          \
  }

FPTU_GET_IMPL(96)
FPTU_GET_IMPL(128)
FPTU_GET_IMPL(160)
FPTU_GET_IMPL(256)
#undef FPTU_GET_IMPL

fptu_error fptu_update_string(fptu_rw *pt, unsigned column, const char *text,
                              std::size_t length) noexcept {
  try {
    const fptu::token id(fptu::genus::text, column, true);
    pt->legacy_update_string(id, fptu::string_view(text, length));
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}

fptu_error fptu_update_opaque(fptu_rw *pt, unsigned column, const void *data,
                              std::size_t length) noexcept {
  try {
    const fptu::token id(fptu::genus::varbin, column, true);
    pt->legacy_update_varbinary(
        id, fptu::string_view(static_cast<const char *>(data), length));
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}

fptu_error fptu_update_opaque_iov(fptu_rw *pt, unsigned column,
                                  const struct iovec value) noexcept {
  return fptu_update_opaque(pt, column, value.iov_base, value.iov_len);
}

fptu_error fptu_update_nested(fptu_rw *pt, unsigned column,
                              fptu_ro ro) noexcept {
  try {
    const fptu::token id(fptu::genus::nested, column, true);
    pt->legacy_update_nested(
        id, fptu::details::tuple_ro::make_from_buffer(
                ro.sys.iov_base, ro.sys.iov_len, nullptr, false));
    return FPTU_OK;
  } catch (const std::exception &e) {
    return fptu_set_error(e).first;
  }
}
