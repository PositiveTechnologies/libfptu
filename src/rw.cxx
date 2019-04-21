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

#include "fast_positive/details/rw.h"
#include "fast_positive/details/api.h"
#include "fast_positive/details/audit.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/exceptions.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/getter.h"
#include "fast_positive/details/meta.h"
#include "fast_positive/details/ro.h"
#include "fast_positive/details/scan.h"
#include "fast_positive/details/schema.h"
#include "fast_positive/details/token.h"

#include "fast_positive/tuples_internal.h"

#include "fast_positive/details/erthink/erthink_optimize4speed.h"

namespace fptu {
namespace details {

namespace {
struct malloc_deleter {
  void operator()(tuple_rw *tuple) { ::free(tuple); }
  void operator()(void *ptr) { ::free(ptr); }
};
} // namespace

void tuple_rw::ensure() {
  const char *trouble = audit();
  if (unlikely(trouble))
    throw_tuple_bad(this, trouble);
}

__hot const char *tuple_rw::audit(const tuple_rw *self) noexcept {
  if (!self)
    return "hollow tuple (nullptr)";

  if (unlikely(self->head_ > self->pivot_))
    return "tuple.head > tuple.pivot";

  if (unlikely(self->pivot_ > self->tail_))
    return "tuple.pivot > tuple.tail";

  if (unlikely(self->tail_ > self->end_))
    return "tuple.tail > tuple.end";

  if (unlikely(self->pivot_ - self->head_ > fptu::max_fields))
    return "tuple.loose_fields > fptu::max_fields";

  if (unlikely(self->tail_ - self->head_ > UINT16_MAX))
    return "tuple.size > max_bytes";

  if (unlikely(self->junk_.holes_count > self->head_))
    return "tuple.junk.holes_count > tuple.index_size";

  if (unlikely(self->junk_.data_units > self->tail_ - self->pivot_))
    return "tuple.junk.data_units > tuple.payload_size";

  audit_flags flags = audit_flags::audit_adjacent_holes;
  if (self->is_sorted())
    flags |= audit_flags::audit_tuple_sorted_loose;
  if (self->have_preplaced())
    flags |= audit_flags::audit_tuple_have_preplaced;

  audit_holes_info holes_info;
  const char *trouble =
      audit_tuple(self->schema_, self->begin_index(), self->pivot(),
                  self->end_data_bytes(), flags, holes_info);
  if (unlikely(trouble))
    return trouble;

  if (unlikely(self->junk_.holes_count != holes_info.holes_count))
    return self->junk_.holes_count ? "tuple.holes_count mismatch"
                                   : "tuple have holes";
  if (unlikely(self->junk_.data_units != holes_info.holes_volume))
    return self->junk_.data_units ? "tuple.junk_volume mismatch"
                                  : "tuple have unaccounted holes";
  return nullptr;
}

#define HERE_GENUS_CASE(RETURN_TYPE, NAME)                                     \
  template <>                                                                  \
  FPTU_API RETURN_TYPE crtp_getter<tuple_rw>::NAME(const token &ident) const { \
    return at(self(), ident).NAME();                                           \
  }
HERE_GENUS_CASE(const tuple_ro *, get_nested)
HERE_GENUS_CASE(string_view, get_string)
HERE_GENUS_CASE(string_view, get_varbinary)
HERE_GENUS_CASE(property_pair, get_property)
HERE_GENUS_CASE(bool, get_bool)
HERE_GENUS_CASE(short, get_enum)
HERE_GENUS_CASE(int8_t, get_i8)
HERE_GENUS_CASE(uint8_t, get_u8)
HERE_GENUS_CASE(int16_t, get_i16)
HERE_GENUS_CASE(uint16_t, get_u16)
HERE_GENUS_CASE(int32_t, get_i32)
HERE_GENUS_CASE(uint32_t, get_u32)
HERE_GENUS_CASE(int64_t, get_i64)
HERE_GENUS_CASE(uint64_t, get_u64)
HERE_GENUS_CASE(float, get_f32)
HERE_GENUS_CASE(double, get_f64)
HERE_GENUS_CASE(decimal64, get_decimal)
HERE_GENUS_CASE(datetime_t, get_datetime)
HERE_GENUS_CASE(const uuid_t &, get_uuid)
HERE_GENUS_CASE(int128_t, get_int128)
HERE_GENUS_CASE(uint128_t, get_uint128)
HERE_GENUS_CASE(const binary96_t &, get_b96)
HERE_GENUS_CASE(const binary128_t &, get_b128)
HERE_GENUS_CASE(const binary160_t &, get_b160)
HERE_GENUS_CASE(const binary192_t &, get_b192)
HERE_GENUS_CASE(const binary224_t &, get_b224)
HERE_GENUS_CASE(const binary256_t &, get_b256)
HERE_GENUS_CASE(const binary320_t &, get_b320)
HERE_GENUS_CASE(const binary384_t &, get_b384)
HERE_GENUS_CASE(const binary512_t &, get_b512)
HERE_GENUS_CASE(const ip_address_t &, get_ip_address)
HERE_GENUS_CASE(mac_address_t, get_mac_address)
HERE_GENUS_CASE(const ip_net_t &, get_ip_net)
HERE_GENUS_CASE(int64_t, get_integer)
HERE_GENUS_CASE(uint64_t, get_unsigned)
HERE_GENUS_CASE(double, get_float)
#undef HERE_GENUS_CASE

template <>
FPTU_API dynamic_collection_ro
crtp_getter<tuple_rw>::collection(const token &ident) const {
  return collect(self(), ident);
}

template <>
FPTU_API dynamic_accessor_ro crtp_getter<tuple_rw>::
operator[](const token &ident) const {
  return at(self(), ident);
}

template <>
FPTU_API bool crtp_getter<tuple_rw>::is_present(const token &ident) const {
  return have(self(), ident);
}

//------------------------------------------------------------------------------

bool tuple_rw::erase(const token &ident) { return remove(ident); }

bool tuple_rw::erase(const dynamic_iterator_rw &it) {
  if (it.exist()) {
    (*it).remove();
    return true;
  }
  return false;
}

size_t tuple_rw::erase(const dynamic_iterator_rw &begin,
                       const dynamic_iterator_rw &end) {
  std::size_t count = 0;
  for (auto it = begin; it != end; ++it)
    count += erase(it) ? 1 : 0;
  return count;
}

size_t tuple_rw::erase(const dynamic_collection_rw &collection) {
  return erase(const_cast<dynamic_collection_rw &>(collection).begin(),
               const_cast<dynamic_collection_rw &>(collection).end());
}

#define HERE_GENUS_CASE(VALUE_TYPE, NAME)                                      \
  FPTU_API void tuple_rw::set_##NAME(const token &ident,                       \
                                     const VALUE_TYPE value) {                 \
    at(ident).set_##NAME(value);                                               \
  }

HERE_GENUS_CASE(tuple_ro *, nested)
HERE_GENUS_CASE(string_view &, string)
HERE_GENUS_CASE(string_view &, varbinary)
HERE_GENUS_CASE(property_pair &, property)
HERE_GENUS_CASE(bool, bool)
HERE_GENUS_CASE(short, enum)
HERE_GENUS_CASE(int8_t, i8)
HERE_GENUS_CASE(uint8_t, u8)
HERE_GENUS_CASE(int16_t, i16)
HERE_GENUS_CASE(uint16_t, u16)
HERE_GENUS_CASE(int32_t, i32)
HERE_GENUS_CASE(uint32_t, u32)
HERE_GENUS_CASE(int64_t, i64)
HERE_GENUS_CASE(uint64_t, u64)
HERE_GENUS_CASE(float, f32)
HERE_GENUS_CASE(double, f64)
HERE_GENUS_CASE(decimal64, decimal)
HERE_GENUS_CASE(datetime_t, datetime)
HERE_GENUS_CASE(uuid_t &, uuid)
HERE_GENUS_CASE(int128_t &, int128)
HERE_GENUS_CASE(uint128_t &, uint128)
HERE_GENUS_CASE(binary96_t &, b96)
HERE_GENUS_CASE(binary128_t &, b128)
HERE_GENUS_CASE(binary160_t &, b160)
HERE_GENUS_CASE(binary192_t &, b192)
HERE_GENUS_CASE(binary224_t &, b224)
HERE_GENUS_CASE(binary256_t &, b256)
HERE_GENUS_CASE(binary320_t &, b320)
HERE_GENUS_CASE(binary384_t &, b384)
HERE_GENUS_CASE(binary512_t &, b512)
HERE_GENUS_CASE(ip_address_t &, ip_address)
HERE_GENUS_CASE(mac_address_t, mac_address)
HERE_GENUS_CASE(ip_net_t &, ip_net)
HERE_GENUS_CASE(int64_t, integer)
HERE_GENUS_CASE(uint64_t, integer)
HERE_GENUS_CASE(int64_t, unsigned)
HERE_GENUS_CASE(uint64_t, unsigned)
HERE_GENUS_CASE(double, float)
#undef HERE_GENUS_CASE

FPTU_API dynamic_collection_rw tuple_rw::collection(const token &ident) {
  return collect(ident);
}

FPTU_API dynamic_accessor_rw tuple_rw::operator[](const token &ident) {
  return at(ident);
}

// template class FPTU_API tuple_rw::accessor_rw<token>;
// template class FPTU_API tuple_rw::iterator_rw<token>;
// template class FPTU_API tuple_rw::collection_rw<token>;

//------------------------------------------------------------------------------

#define HERE_GENUS_CASE(VALUE_TYPE, NAME, GENUS)                               \
  FPTU_API dynamic_iterator_rw tuple_rw::insert_##NAME(                        \
      const token &ident, const VALUE_TYPE value) {                            \
    field_loose *field = append_field<token, genus::GENUS>(value, ident);      \
    return dynamic_iterator_rw(this, field, ident);                            \
  }

HERE_GENUS_CASE(tuple_ro *, nested, nested)
HERE_GENUS_CASE(string_view &, string, text)
HERE_GENUS_CASE(string_view &, varbinary, varbin)
HERE_GENUS_CASE(property_pair &, property, property)
HERE_GENUS_CASE(bool, bool, boolean)
HERE_GENUS_CASE(short, enum, enumeration)
HERE_GENUS_CASE(int8_t, i8, i8)
HERE_GENUS_CASE(uint8_t, u8, u8)
HERE_GENUS_CASE(int16_t, i16, i16)
HERE_GENUS_CASE(uint16_t, u16, u16)
HERE_GENUS_CASE(int32_t, i32, i32)
HERE_GENUS_CASE(uint32_t, u32, u32)
HERE_GENUS_CASE(int64_t, i64, i64)
HERE_GENUS_CASE(uint64_t, u64, u64)
HERE_GENUS_CASE(float, f32, f32)
HERE_GENUS_CASE(double, f64, f64)
HERE_GENUS_CASE(decimal64, decimal, d64)
HERE_GENUS_CASE(datetime_t, datetime, t64)
HERE_GENUS_CASE(binary96_t &, b96, b96)
HERE_GENUS_CASE(binary128_t &, b128, b128)
HERE_GENUS_CASE(binary160_t &, b160, b160)
HERE_GENUS_CASE(binary192_t &, b192, b192)
HERE_GENUS_CASE(binary224_t &, b224, b224)
HERE_GENUS_CASE(binary256_t &, b256, b256)
HERE_GENUS_CASE(binary320_t &, b320, b320)
HERE_GENUS_CASE(binary384_t &, b384, b384)
HERE_GENUS_CASE(binary512_t &, b512, b512)
HERE_GENUS_CASE(ip_address_t &, ip_address, ip)
HERE_GENUS_CASE(mac_address_t, mac_address, mac)
HERE_GENUS_CASE(ip_net_t &, ip_net, ipnet)
#undef HERE_GENUS_CASE

FPTU_API dynamic_iterator_rw tuple_rw::insert_uuid(const token &ident,
                                                   const uuid_t &value) {
  field_loose *field = append_field<token, genus::b128>(value.bin128, ident);
  return dynamic_iterator_rw(this, field, ident);
}

FPTU_API dynamic_iterator_rw tuple_rw::insert_float(const token &ident,
                                                    const double value) {
  if (ident.type() == f32) {
    if (!std::isinf(value) &&
        unlikely(value > std::numeric_limits<float>::max() ||
                 value < std::numeric_limits<float>::lowest()))
      throw_value_range();
    return insert_f32(ident, static_cast<float>(value));
  } else {
    return insert_f64(ident, value);
  }
}

FPTU_API dynamic_iterator_rw tuple_rw::insert_integer(const token &ident,
                                                      const int64_t value) {
  switch (ident.type()) {
  default:
    throw_type_mismatch();
  case i8:
    if (unlikely(value != static_cast<int8_t>(value)))
      throw_value_range();
    return insert_i8(ident, static_cast<int8_t>(value));
  case i16:
    if (unlikely(value != static_cast<int16_t>(value)))
      throw_value_range();
    return insert_i16(ident, static_cast<int16_t>(value));
  case i32:
    if (unlikely(value != static_cast<int32_t>(value)))
      throw_value_range();
    return insert_i32(ident, static_cast<int32_t>(value));
  case i64:
    return insert_i64(ident, value);
  }
}

FPTU_API dynamic_iterator_rw tuple_rw::insert_integer(const token &ident,
                                                      const uint64_t value) {
  if (unlikely(value > uint64_t(std::numeric_limits<int64_t>::max())))
    throw_value_range();
  return insert_integer(ident, static_cast<int64_t>(value));
}

FPTU_API dynamic_iterator_rw tuple_rw::insert_unsigned(const token &ident,
                                                       const uint64_t value) {
  switch (ident.type()) {
  default:
    throw_type_mismatch();
  case u8:
    if (unlikely(value != static_cast<uint8_t>(value)))
      throw_value_range();
    return insert_u8(ident, static_cast<uint8_t>(value));
  case u16:
    if (unlikely(value != static_cast<uint16_t>(value)))
      throw_value_range();
    return insert_u16(ident, static_cast<uint16_t>(value));
  case u32:
    if (unlikely(value != static_cast<uint32_t>(value)))
      throw_value_range();
    return insert_u32(ident, static_cast<uint32_t>(value));
  case u64:
    return insert_u64(ident, value);
  }
}

FPTU_API dynamic_iterator_rw tuple_rw::insert_unsigned(const token &ident,
                                                       const int64_t value) {
  if (unlikely(value < 0))
    throw_value_range();
  return insert_integer(ident, static_cast<uint64_t>(value));
}

FPTU_API dynamic_iterator_rw tuple_rw::insert_int128(const token &ident,
                                                     const int128_t &value) {
  if (ident.type() == b128)
    return insert_b128(
        ident, *erthink::constexpr_pointer_cast<const binary128_t *>(&value));
  return insert_integer(ident, int64_t(value));
}

FPTU_API dynamic_iterator_rw tuple_rw::insert_uint128(const token &ident,
                                                      const uint128_t &value) {
  if (ident.type() == b128)
    return insert_b128(
        ident, *erthink::constexpr_pointer_cast<const binary128_t *>(&value));
  return insert_unsigned(ident, uint64_t(value));
}

//------------------------------------------------------------------------------

void tuple_rw::reset() noexcept {
  /* sanity check for corruption */
  if (unlikely(pivot_ < 1 || pivot_ > fptu::max_fields || pivot_ > end_ ||
               end_ > bytes2units(fptu::buffer_limit)))
    throw_tuple_bad(this);

  junk_.both = 0;
  head_ = tail_ = pivot_;
  if (schema_) {
    /* sanity check for corruption */
    if (unlikely(schema_->preplaced_units() > end_ - tail_))
      throw_tuple_bad(this);
    tail_ += unsigned(schema_->preplaced_units());
    std::memcpy(pivot(), schema_->preplaced_init_image(),
                schema_->preplaced_bytes());
  }
}

const tuple_ro *tuple_rw::take_asis() const {
  if (unlikely(tail_ - head_ >= UINT16_MAX || pivot_ - head_ > max_fields))
    throw_tuple_too_large();

  stretchy_value_tuple *header =
      erthink::constexpr_pointer_cast<stretchy_value_tuple *>(
          const_cast<tuple_rw *>(this)->begin_index()) -
      1;
  static_assert(sizeof(stretchy_value_tuple) == sizeof(unit_t), "Oops");
  assert(header >=
         erthink::constexpr_pointer_cast<const stretchy_value_tuple *>(
             &reserve_for_RO_header));
  header->brutto_units = (uint16_t)(tail_ - head_ + 1);
  header->set_index_size_and_flags(
      pivot_ - head_,
      (is_sorted() ? stretchy_value_tuple::sorted_flag : 0) |
          (have_preplaced() ? stretchy_value_tuple::preplaced_flag : 0));
  return erthink::constexpr_pointer_cast<tuple_ro *>(header);
}

std::pair<const tuple_ro *, bool> tuple_rw::take_optimized() {
  const bool interators_invalidated = optimize();
  return std::make_pair(take_asis(), interators_invalidated);
}

tuple_rw::~tuple_rw() {
  if (likely(buffer_offset_)) {
    get_buffer()->detach();
  } else {
    /* compatibility with legacy C API */
    ::free(this);
  }
}

inline tuple_rw::tuple_rw(int buffer_offset, std::size_t buffer_size,
                          std::size_t items_limit, const fptu::schema *schema)
    : schema_(schema), buffer_offset_(buffer_offset), options_(0) {
  const auto required_space = estimate_required_space(items_limit, 0, schema);
  if (unlikely(required_space > buffer_size || buffer_size > buffer_limit))
    throw_invalid_argument();

  end_ = unsigned(buffer_size - sizeof(tuple_rw)) >>
         fptu::fundamentals::unit_shift;
  head_ = tail_ = pivot_ = unsigned(items_limit);
  if (schema_) {
    tail_ += unsigned(schema_->preplaced_units());
    std::memcpy(pivot(), schema_->preplaced_init_image(),
                schema_->preplaced_bytes());
  }
}

tuple_rw *tuple_rw::create_new(std::size_t items_limit, std::size_t data_bytes,
                               const fptu::schema *schema,
                               const hippeus::buffer_tag &allot_tag) {
  const std::size_t buffer_size =
      estimate_required_space(items_limit, data_bytes, schema);
  if (allot_tag) {
    hippeus::buffer_ptr buffer(
        hippeus::buffer::borrow(allot_tag, buffer_size, true));

    tuple_rw *rw = new (buffer->data()) tuple_rw(
        get_buffer_offset(buffer.get()), buffer->size(), items_limit, schema);
    buffer.release();
    return rw;
  }

  std::unique_ptr<void, malloc_deleter> ptr(::malloc(buffer_size));
  if (unlikely(!ptr))
    return nullptr;
  tuple_rw *rw = new (ptr.get()) tuple_rw(0, buffer_size, items_limit, schema);
  ptr.release();
  return rw;
}

tuple_rw *tuple_rw::create_legacy(void *buffer_space, std::size_t buffer_size,
                                  std::size_t items_limit) {
  return new (buffer_space) tuple_rw(0, buffer_size, items_limit, nullptr);
}

inline tuple_rw::tuple_rw(const audit_holes_info &holes_info,
                          const tuple_ro *ro, int buffer_offset,
                          std::size_t buffer_size, std::size_t more_items,
                          const fptu::schema *schema)
    : schema_(schema), buffer_offset_(buffer_offset), options_(0) {
  /* Во избежание двойной работы здесь НЕ проверяется корректность переданного
   * экземпляра tuple_ro и его соответствие схеме. Такая проверка должна
   * выполняться один раз явным вызовом валидирующих функций. Например, внутри
   * конструкторов безопасных объектов. */
  if (unlikely(more_items > max_fields))
    throw_invalid_argument("items > fptu::max_fields");
  if (unlikely(buffer_size > fptu::buffer_limit))
    throw_invalid_argument("buffer_bytes > fptu::buffer_limit");

  const std::size_t have_items = ro->index_size();
  const std::size_t reserve_items =
      std::min(size_t(fptu::max_fields), have_items + more_items);
  const std::size_t space_needed = tuple_rw::estimate_required_space(
      reserve_items, ro->payload_bytes(), schema);

  end_ = unsigned(buffer_size - sizeof(tuple_rw)) >>
         fptu::fundamentals::unit_shift;
  pivot_ = unsigned(reserve_items);
  if (unlikely(buffer_size < space_needed)) {
    head_ = tail_ = pivot_;
    throw_insufficient_space(reserve_items, space_needed);
  }

  head_ = pivot_ - unsigned(have_items);
  tail_ = pivot_ + unsigned(bytes2units(ro->payload_units()));
  junk_.holes_count = holes_info.holes_count;
  junk_.data_units = holes_info.holes_volume;

  assert((void *)(&area_[head_]) == (void *)begin_index());
  assert((void *)(ro->flat + 1) == (void *)ro->begin_index());
  /* небольшая возня для гашения предупреждений о memcpy-копировании объектов
   * с не-тривиальными (удаленными) конструкторами */
  std::memcpy(&area_[head_] /* begin_index() */,
              ro->flat + 1 /* ro->begin_index() */,
              ro->size() - fptu::unit_size);

  /* Не представляется рациональным реализовывать здесь автоматическую
   * оптимизацию кортежа (дефрагментацию и сортировку индекса). С одной
   * стороны, это позволит избежать одного копирования данных. Однако,
   * появление фрагментированных кортежей должно быть редкой ситуацией,
   * а автоматическая оптимизация может быть НЕ лучшей стратегией. */
}

tuple_rw *tuple_rw::create_from_ro(const audit_holes_info &holes_info,
                                   const tuple_ro *ro, std::size_t more_items,
                                   std::size_t more_payload,
                                   const fptu::schema *schema,
                                   const hippeus::buffer_tag &allot_tag) {
  /* Во избежание двойной работы здесь НЕ проверяется корректность переданного
   * экземпляра tuple_ro и его соответствие схеме. Такая проверка должна
   * выполняться один раз явным вызовом валидирующих функций. Например, внутри
   * конструкторов безопасных объектов. */
  const std::size_t buffer_size =
      estimate_required_space(ro, more_items, more_payload, schema);
  if (allot_tag) {
    hippeus::buffer_ptr buffer(
        hippeus::buffer::borrow(allot_tag, buffer_size, true));
    tuple_rw *rw = new (buffer->data())
        tuple_rw(holes_info, ro, get_buffer_offset(buffer.get()),
                 buffer->size(), more_items, schema);
    buffer.release();
    return rw;
  }

  std::unique_ptr<void, malloc_deleter> ptr(::malloc(buffer_size));
  if (unlikely(!ptr))
    return nullptr;
  tuple_rw *rw = new (ptr.get())
      tuple_rw(holes_info, ro, 0, buffer_size, more_items, schema);
  ptr.release();
  return rw;
}

tuple_rw *tuple_rw::fetch_legacy(const audit_holes_info &holes_info,
                                 const tuple_ro *ro, void *buffer_space,
                                 std::size_t buffer_size,
                                 std::size_t more_items) {
  return new (buffer_space)
      tuple_rw(holes_info, ro, 0, buffer_size, more_items, nullptr);
}

tuple_rw *tuple_rw::create_from_buffer(const void *ptr, std::size_t bytes,
                                       std::size_t more_items,
                                       std::size_t more_payload,
                                       const fptu::schema *schema,
                                       const hippeus::buffer_tag &allot_tag) {
  fptu::details::audit_holes_info holes_info;
  const char *trouble =
      fptu::details::tuple_ro::audit(ptr, bytes, schema, holes_info);

  if (unlikely(trouble))
    throw_tuple_bad(schema, ptr, bytes, trouble);

  return create_from_ro(holes_info, static_cast<const tuple_ro *>(ptr),
                        more_items, more_payload, schema, allot_tag);
}

} // namespace details

// template class tuple_crtp_writer<tuple_rw_fixed>;

FPTU_API __cold __noreturn void
tuple_rw_fixed::throw_managed_buffer_required() const {
  throw managed_buffer_required(
      "tuple_rw_fixed: managed 1Hippeus's buffer required for instance");
}

} // namespace fptu
