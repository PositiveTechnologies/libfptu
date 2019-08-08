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

#include "fast_positive/tuples/details/ro.h"
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

#include "fast_positive/tuples/internal.h"
#include <algorithm>

#include "fast_positive/erthink/erthink_optimize4speed.h"

namespace fptu {

//------------------------------------------------------------------------------

token loose_iterator_ro::field_token(const fptu::schema *schema) const {
  if (likely(schema)) {
    const token ident = field_token_nothow(schema);
    if (likely(ident.is_valid())) {
      assert(ident.is_loose());
      return ident;
    }
  }
  throw schema_no_such_field();
}

token field_iterator_ro::field_token() const {
  if (likely(schema_)) {
    const token ident = field_token_nothrow();
    if (likely(ident.is_valid())) {
      assert(ident.is_preplaced() == on_preplaced_field());
      return ident;
    }
  }
  throw schema_no_such_field();
}

const token field_iterator_ro::preplaced_token() const {
  if (likely(schema_)) {
    const token ident = schema_->by_offset(preplaced_offset());
    if (likely(ident.is_valid())) {
      assert(ident.is_preplaced());
      return ident;
    }
  }
  throw schema_no_such_field();
}

bool field_iterator_ro::operator==(const token &ident) const {
  if (on_preplaced_field())
    return ident.is_preplaced() && preplaced_token() == ident;
  return ident.is_loose() &&
         loose()->genus_and_id == details::tag2genus_and_id(ident.tag());
}

field_iterator_ro &field_iterator_ro::operator++() {
  ptrdiff_t offset = preplaced_offset();
  if (offset >= 0) {
    /* итератор в секторе preplaced-полей */
    assert(schema_);
    while (true) {
      /* получаем токен следующего поля */
      const token ident = schema_->next_by_offset(offset);
      if (unlikely(!ident.is_valid() || !ident.is_preplaced()))
        /* дошли до конца preplaced */
        break;

      /* устанавливаем итератор на следующее preplaced-поле */
      offset = ident.preplaced_offset();
      field_ = static_cast<const char *>(pivot_) + offset;

      /* пропускаем null-евые поля */
      if (likely(!preplaced()->is_null(ident.tag())))
        return *this;
    }

    /* переходим к loose-полям */
    field_ = static_cast<const details::field_loose *>(pivot_) - 1;
  } else {
    /* итератор в секторе loose-полей */
    /* переходим к следующему loose-полю в порядке итерирования */
    field_ = loose() - 1;
  }

  return *this;
}

field_iterator_ro &field_iterator_ro::operator--() {
  ptrdiff_t offset = preplaced_offset();
  if (offset < 0) {
    /* итератор в секторе loose-полей */
    /* переходим к предыдущему loose-полю в порядке итерирования */
    field_ = loose() + 1;
    if (field_ != pivot_)
      return *this;

    /* переходим к последнему preplaced-полю,
     * корректировка смещения будет дальше */
    assert(schema_);
    offset = schema_->preplaced_bytes();
    if (offset == 0)
      return *this /* нет preplaced-полей */;
  } else {
    /* итератор в секторе preplaced-полей */
    assert(schema_);
    assert(offset != 0 /* нельзя декремировать begin */);
  }

  while (true) {
    /* получаем токен предыдущего поля */
    const token ident = schema_->prev_by_offset(offset);
    assert(ident.is_valid() && ident.is_preplaced());

    /* устанавливаем итератор на предыдущее preplaced-поле */
    offset = ident.preplaced_offset();
    field_ = static_cast<const char *>(pivot_) + offset;

    /* останавливаемся если дошли до первого поля */
    if (!offset)
      break;

    /* пропускаем null-евые поля */
    if (!preplaced()->is_null(ident.tag()))
      break;
  }

  return *this;
}

field_iterator_ro
field_iterator_ro::begin(const void *pivot,
                         const fptu::schema *schema) noexcept {
  const auto last_loose = static_cast<const details::field_loose *>(pivot) - 1;
  if (schema && schema->number_of_preplaced()) {
    field_iterator_ro iter(static_cast<const details::field_preplaced *>(pivot),
                           pivot, schema);
    /* пропускаем null-евые preplaced-поля */
    auto ident = schema->tokens().begin();
    assert(ident->is_valid() && ident->preplaced_offset() == 0);
    const auto detent = ident + schema->number_of_preplaced();
    while (unlikely(iter.preplaced()->is_null(ident->tag()))) {
      if (++ident == detent) {
        /* дошли до конца preplaced, переходим к первому loose-полю */
        iter.field_ = last_loose;
        break;
      }

      /* устанавливаем итератор на следующее preplaced-поле */
      assert(ident->is_valid());
      iter.field_ =
          static_cast<const char *>(pivot) + ident->preplaced_offset();
    }
    return iter;
  } else {
    /* нет схемы, preplaced-полей быть не может */
    /* возращаем итератор на первое loose-поле;
     * если loose-полей нет, то результат будет равен end() */
    return field_iterator_ro(last_loose, pivot, schema);
  }
}

//------------------------------------------------------------------------------

namespace details {

const tuple_ro *tuple_ro::make_from_buffer(const void *ptr, std::size_t bytes,
                                           const schema *schema,
                                           bool skip_validation) {
  if (unlikely(!ptr || bytes < sizeof(stretchy_value_tuple))) {
    if (skip_validation)
      return nullptr;
    throw_tuple_hollow();
  }

  if (!skip_validation) {
    const char *trouble = audit(ptr, bytes, schema, false);
    if (unlikely(trouble))
      throw_tuple_bad(schema, ptr, bytes, trouble);
  }

  return static_cast<const tuple_ro *>(ptr);
}

//------------------------------------------------------------------------------

#define HERE_CRTP_MAKE(RETURN_TYPE, NAME)                                      \
  template <>                                                                  \
  FPTU_API __pure_function RETURN_TYPE crtp_getter<tuple_ro>::NAME(            \
      const token &ident) const {                                              \
    return at(self(), ident).NAME();                                           \
  }
HERE_CRTP_MAKE(const tuple_ro *, get_nested)
HERE_CRTP_MAKE(string_view, get_string)
HERE_CRTP_MAKE(string_view, get_varbinary)
HERE_CRTP_MAKE(property_pair, get_property)
HERE_CRTP_MAKE(bool, get_bool)
HERE_CRTP_MAKE(short, get_enum)
HERE_CRTP_MAKE(int8_t, get_i8)
HERE_CRTP_MAKE(uint8_t, get_u8)
HERE_CRTP_MAKE(int16_t, get_i16)
HERE_CRTP_MAKE(uint16_t, get_u16)
HERE_CRTP_MAKE(int32_t, get_i32)
HERE_CRTP_MAKE(uint32_t, get_u32)
HERE_CRTP_MAKE(int64_t, get_i64)
HERE_CRTP_MAKE(uint64_t, get_u64)
HERE_CRTP_MAKE(float, get_f32)
HERE_CRTP_MAKE(double, get_f64)
HERE_CRTP_MAKE(decimal64, get_decimal)
HERE_CRTP_MAKE(datetime_t, get_datetime)
HERE_CRTP_MAKE(const uuid_t &, get_uuid)
HERE_CRTP_MAKE(int128_t, get_int128)
HERE_CRTP_MAKE(uint128_t, get_uint128)
HERE_CRTP_MAKE(const binary96_t &, get_bin96)
HERE_CRTP_MAKE(const binary128_t &, get_bin128)
HERE_CRTP_MAKE(const binary160_t &, get_bin160)
HERE_CRTP_MAKE(const binary192_t &, get_bin192)
HERE_CRTP_MAKE(const binary224_t &, get_bin224)
HERE_CRTP_MAKE(const binary256_t &, get_bin256)
HERE_CRTP_MAKE(const binary320_t &, get_bin320)
HERE_CRTP_MAKE(const binary384_t &, get_bin384)
HERE_CRTP_MAKE(const binary512_t &, get_bin512)
HERE_CRTP_MAKE(const ip_address_t &, get_ip_address)
HERE_CRTP_MAKE(mac_address_t, get_mac_address)
HERE_CRTP_MAKE(const ip_net_t &, get_ip_net)
HERE_CRTP_MAKE(int64_t, get_integer)
HERE_CRTP_MAKE(uint64_t, get_unsigned)
HERE_CRTP_MAKE(double, get_float)
#undef HERE_CRTP_MAKE

template <>
FPTU_API __pure_function dynamic_collection_ro
crtp_getter<tuple_ro>::collection(const token &ident) const {
  return collect(self(), ident);
}

template <>
FPTU_API __pure_function dynamic_accessor_ro crtp_getter<tuple_ro>::
operator[](const token &ident) const {
  return at(self(), ident);
}

template <>
FPTU_API __pure_function bool
crtp_getter<tuple_ro>::is_present(const token &ident) const {
  return have(self(), ident);
}

// template class FPTU_API_TYPE accessor_ro<token>;
// template class FPTU_API_TYPE collection_iterator_ro<token>;
// template class FPTU_API_TYPE collection_ro<token>;

} // namespace details

// template class tuple_crtp_reader<tuple_ro_weak>;

__cold __noreturn void tuple_ro_managed::throw_buffer_mismatch() const {
  if (!get_buffer())
    throw managed_buffer_required(
        "tuple_ro_managed: managed 1Hippeus's buffer required for instance");
  else
    throw logic_error(
        get_impl()
            ? "tuple_ro_managed: tuple bounds are out of the managed buffer"
            : "tuple_ro_managed: tuple is nullptr, but buffer is provided");
}

//------------------------------------------------------------------------------

string_view /* preplaced_string::value_type */
preplaced_string::value() const {
  if (unlikely(nil()))
    throw_field_absent();

  return traits::read(payload());
}

string_view /* preplaced_varbin::value_type */
preplaced_varbin::value() const {
  if (unlikely(nil()))
    throw_field_absent();

  return traits::read(payload());
}

tuple_ro_weak /* preplaced_nested::value_type */
preplaced_nested::value() const {
  if (unlikely(nil()))
    throw_field_absent();

  return tuple_ro_weak(
      erthink::constexpr_pointer_cast<const details::tuple_ro *>(
          traits::read(payload())));
}

property_pair /* preplaced_property::value_type */
preplaced_property::value() const {
  if (unlikely(nil()))
    throw_field_absent();

  return traits::read(payload());
}

//------------------------------------------------------------------------------

} // namespace fptu
