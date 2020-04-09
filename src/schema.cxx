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

#include "fast_positive/tuples/schema.h"
#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/details/bug.h"
#include "fast_positive/tuples/essentials.h"
#include "fast_positive/tuples/token.h"

#include <algorithm>
#include <unordered_map>

namespace fptu {

inline schema::schema()
    : number_of_preplaced_(0), number_of_stretchy_preplaced_(0) {}
schema::~schema() {}

namespace {
struct schema_impl : public schema {
  struct hash_name {
    bool operator()(const string_view &name) const cxx11_noexcept {
      return name.hash_value();
    }
    bool operator()(const std::string &name) const cxx11_noexcept {
      return operator()(string_view(name));
    }
  };

  //----------------------------------------------------------------------------

  using map_name2token = std::unordered_map<std::string, token>;
  using map_token2name =
      std::unordered_map<token, string_view, token::hash, token::same>;
  map_name2token name2token_;
  map_token2name token2name_;

  void purge() cxx11_noexcept {
    sorted_tokens_.clear();
    number_of_preplaced_ = number_of_stretchy_preplaced_ = 0;
    preplaced_image_.clear();
    name2token_.clear();
    token2name_.clear();
  }

  void add_definition(std::string &&name, const token &ident,
                      const void *initial_value);

  unsigned get_next_loose_id(fptu::genus type) const;

  token define_preplaced(std::string &&name, fptu::genus type,
                         const bool discernible_null, const bool saturation,
                         const void *initial_value) override;

  token define_preplaced_fixed_opacity(std::string &&field_name,
                                       std::size_t size, std::size_t align,
                                       const void *initial_value) override;
  token define_loose(std::string &&field_name, fptu::genus type,
                     const bool collection, const bool discernible_null,
                     const bool saturated) override;
  token import_definition(std::string &&field_name, const token &,
                          const void *initial_value,
                          const bool renominate) override;

  token get_token_nothrow(
      const string_view &field_name, const boolean_option discernible_null,
      const boolean_option saturated) const cxx11_noexcept override;
  token get_token_nothrow(
      const token &inlay_token, const string_view &inner_name,
      const boolean_option discernible_null,
      const boolean_option saturated) const cxx11_noexcept override;
  string_view
  get_name_nothrow(const token &ident) const cxx11_noexcept override;
};

static bool is_inside(const std::ptrdiff_t point, const token &ident) {
  return point >= ident.preplaced_offset() &&
         point < ident.preplaced_offset() + ptrdiff_t(ident.preplaced_size());
}

static bool is_crossing(const token &a, const token &b) {
  return is_inside(a.preplaced_offset(), b) !=
         is_inside(a.preplaced_offset() + a.preplaced_size() - 1, b);
}

static bool is_overlapped(const token &a, const token &b) {
  if (a.preplaced_offset() + ptrdiff_t(a.preplaced_size()) <=
      b.preplaced_offset())
    return false;
  if (a.preplaced_offset() >=
      b.preplaced_offset() + ptrdiff_t(b.preplaced_size()))
    return false;
  return true;
}

unsigned schema_impl::get_next_loose_id(fptu::genus type) const {
  const token ident(fptu::genus(type), details::max_ident, false, false, false);
  const auto greater_than =
      std::upper_bound(sorted_tokens_.begin(), sorted_tokens_.end(), ident);
  if (greater_than != sorted_tokens_.begin()) {
    const auto less_or_equal = std::prev(greater_than);
    if (!less_or_equal->is_preplaced() && less_or_equal->type() == type)
      return less_or_equal->id() + 1;
  }
  return 0;
}

void schema_impl::add_definition(std::string &&name, const token &ident,
                                 const void *initial_value) {
  if (unlikely(name.length() > 42))
    throw_invalid_argument("field name is too long");
  if (unlikely(name2token_.count(name)))
    throw_schema_definition_error("fptu: field with given name already exists");
  if (unlikely(token2name_.count(ident)))
    throw_schema_definition_error(
        "fptu: field with corresponding token already exists");

  if (ident.is_preplaced()) {
    /* проверяем перекрытие полей, которые могут быть при слиянии
     * схем с renominate == FALSE */
    for (const auto &scan : sorted_tokens_) {
      if (!scan.is_preplaced())
        break;

      /* одно поле никогда не должно пересекать границы другого */
      if (unlikely(is_crossing(ident, scan)))
        throw_schema_definition_error(
            "preplaced field is crossing with another");

      /* наложение допустимо только для opacity-структур,
       * которые кодируются как preplaced-holes */
      if (ident.type() != genus::hole && scan.type() != genus::hole &&
          unlikely(is_overlapped(ident, scan)))
        throw_schema_definition_error(
            "preplaced field is overlapped by another");
    }
    preplaced_image_.reserve(ident.preplaced_offset() + ident.preplaced_size() -
                             preplaced_image_.size());
  }
  sorted_tokens_.reserve(sorted_tokens_.size() + 1);
  name2token_.reserve(name2token_.size() + 1);
  token2name_.reserve(token2name_.size() + 1);

  try {
    if (ident.is_preplaced()) {
      if (ident.preplaced_offset() > ptrdiff_t(preplaced_image_.size()))
        preplaced_image_.append(
            ident.preplaced_offset() - preplaced_image_.size(), '\0');
      if (initial_value)
        preplaced_image_.append(static_cast<const char *>(initial_value),
                                ident.preplaced_size());
      else {
        preplaced_image_.append(ident.preplaced_size(), '\0');
        const auto initial_value_ptr =
            erthink::constexpr_pointer_cast<details::field_preplaced *>(
                const_cast<char *>(preplaced_image_.data()) +
                ident.preplaced_offset());
        details::preplaced_erase(ident.type(), initial_value_ptr,
                                 ident.is_discernible_null());
      }
      number_of_stretchy_preplaced_ += ident.is_stretchy();
      number_of_preplaced_ += 1;
    }

    sorted_tokens_.push_back(ident);
    auto const pair = name2token_.emplace(std::move(name), ident);
    assert(pair.second);
    token2name_.emplace(ident, string_view(pair.first->first));
    assert(name2token_.size() == sorted_tokens_.size());
    assert(sorted_tokens_.size() == token2name_.size());
  } catch (...) {
    /* исключений быть не должно, но если что-то пошло не так,
     * то на всякий случай стираем всё, чтобы не заботиться о
     * взаимном соответствии потрохов схемы. */
    purge();
    throw;
  }

  std::sort(sorted_tokens_.begin(), sorted_tokens_.end());
}

token schema_impl::define_preplaced(std::string &&name, fptu::genus type,
                                    const bool discernible_null,
                                    const bool saturation,
                                    const void *initial_value) {
  if (unlikely(type >= genus::hole))
    throw_invalid_argument("invalid field type");

  const std::size_t length = details::preplaced_bytes_dynamic(type);
  const std::size_t align = (length > fundamentals::unit_size)
                                ? size_t(fundamentals::unit_size)
                                : length;
  assert(align > 0 && align <= 256 && utils::is_power2(align));
  const std::size_t unaligned_offset = preplaced_image_.size();
  const std::size_t aligned_offset =
      unaligned_offset + ((align - unaligned_offset) & (align - 1));
  if (unlikely(aligned_offset > details::max_preplaced_offset))
    throw_schema_definition_error("fptu: too many preplaced fields");

  const token ident(aligned_offset, type, discernible_null, saturation);
  add_definition(std::move(name), ident, initial_value);
  return ident;
}

token schema_impl::define_preplaced_fixed_opacity(std::string &&name,
                                                  std::size_t size,
                                                  std::size_t align,
                                                  const void *initial_value) {
  if (unlikely(size >= fptu::max_preplaced_size))
    throw_invalid_argument("preplaced field is too large");
  if (unlikely(size < 1))
    throw_invalid_argument("illegal preplaced field size");
  if (unlikely(align >= std::alignment_of<std::max_align_t>::value))
    throw_invalid_argument("requested alignment cannot be satisfied");
  if (align == 0)
    align = (size > fundamentals::unit_size) ? size_t(fundamentals::unit_size)
                                             : size;
  else if (unlikely(!utils::is_power2(align)))
    throw_invalid_argument("alignment must be a power of 2");

  assert(align > 0 && align <= 256 && utils::is_power2(align));
  const std::size_t unaligned_offset = preplaced_image_.size();
  const std::size_t aligned_offset =
      unaligned_offset + ((align - unaligned_offset) & (align - 1));
  if (unlikely(aligned_offset > details::max_preplaced_offset))
    throw_schema_definition_error("fptu: too many preplaced fields");

  const token ident(aligned_offset, genus::hole, false, false);
  add_definition(std::move(name), ident, initial_value);
  return ident;
}

token schema_impl::define_loose(std::string &&name, fptu::genus type,
                                const bool collection,
                                const bool discernible_null,
                                const bool saturated) {
  if (unlikely(type >= genus::hole))
    throw_invalid_argument("invalid field type");

  const unsigned id = get_next_loose_id(type);
#ifndef NDEBUG
  unsigned check = 0;
  for (const auto &item : sorted_tokens_)
    if (item.is_loose() && item.type() == type)
      check = std::max(check, item.id() + 1);
  assert(check == id);
#endif
  if (unlikely(id >= details::loose_end))
    throw_schema_definition_error("fptu: too many loose fields");

  const token ident(type, id, collection, discernible_null, saturated);
  add_definition(std::move(name), ident, nullptr);
  return ident;
}

token schema_impl::import_definition(std::string &&name, const token &ident,
                                     const void *initial_value,
                                     const bool renominate) {
  if (!renominate) {
    add_definition(std::move(name), ident, initial_value);
    return ident;
  } else if (ident.is_preplaced()) {
    if (unlikely(ident.type() == genus::hole))
      throw_schema_definition_error("fptu: opacity structural preplaced fields "
                                    "couldn't be imported with renomination");
    return define_preplaced(std::move(name), ident.type(),
                            ident.is_discernible_null(), ident.is_saturated(),
                            initial_value);
  } else {
    return define_loose(std::move(name), ident.type(), ident.is_collection(),
                        ident.is_discernible_null(), ident.is_saturated());
  }
}

token schema_impl::get_token_nothrow(
    const string_view &name, const boolean_option discernible_null,
    const boolean_option saturated) const cxx11_noexcept {
  const auto it = name2token_.find(name);
  if (it == name2token_.end())
    return token();

  token ident = it->second;
  assert(token2name_.at(it->second) == name);
  if (discernible_null != boolean_option::option_default)
    ident.enforce_discernible_null(discernible_null ==
                                   boolean_option::option_enforce_true);
  if (saturated != boolean_option::option_default)
    ident.enforce_saturation(saturated == boolean_option::option_enforce_true);
  return ident;
}

token schema_impl::get_token_nothrow(
    const token &inlay_token, const string_view &inner_name,
    const boolean_option discernible_null,
    const boolean_option saturated) const cxx11_noexcept {
  (void)inlay_token;
  (void)inner_name;
  (void)discernible_null;
  (void)saturated;
  FPTU_NOT_IMPLEMENTED();
  return token();
}

string_view
schema_impl::get_name_nothrow(const token &ident) const cxx11_noexcept {
  const auto it = token2name_.find(ident);
  if (it == token2name_.end()) {
    assert(string_view().nil());
    return string_view();
  }
  assert(name2token_.at(it->second) == ident);
  return it->second;
}

} // namespace

std::unique_ptr<schema> schema::create() {
  return std::unique_ptr<schema>(new schema_impl());
}

__hot schema::token_vector::const_iterator
schema::search_preplaced(const ptrdiff_t offset) const cxx11_noexcept {
  assert(offset >= 0 && size_t(offset) <= preplaced_bytes());

  size_t span = number_of_preplaced();
  assert(span <= sorted_tokens_.size());

  /* Смещение prelaced-поля в старших битах тэга и токена, поэтому для поиска
   * нормализация НЕ требуется, ибо дополнительные флаги не меняют порядка */
  const details::tag_t tag(
      static_cast<details::tag_t>(offset << details::tag_bits::offset_shift));
  assert(tag >> details::tag_bits::offset_shift == size_t(offset));

  auto iter = sorted_tokens_.begin();
  while (span > 3) {
    const auto top = span;
    span >>= 1;
    const auto middle = iter + span;
    assert(middle->is_preplaced());
    if (middle->tag() < tag) {
      iter = middle + 1;
      span = top - span - 1;
    }
  }

  switch (span) {
  case 3:
    assert(iter->is_preplaced());
    if (iter->tag() >= tag)
      break;
    ++iter;
    __fallthrough /* fall through */;
  case 2:
    assert(iter->is_preplaced());
    if (iter->tag() >= tag)
      break;
    ++iter;
    __fallthrough /* fall through */;
  case 1:
    assert(iter->is_preplaced());
    if (iter->tag() >= tag)
      break;
    ++iter;
    __fallthrough /* fall through */;
  case 0:
    break;
  default:
    assert(false);
    __unreachable();
  }
  return iter;
}

__hot token
schema::by_loose(const details::field_loose *field) const cxx11_noexcept {
  size_t span = sorted_tokens_.size() - number_of_preplaced();
  if (likely(span)) {
    auto iter = sorted_tokens_.begin() + number_of_preplaced();
    /* id в теге и токене loose-поля расположено в младших битах, поэтому для
     * поиска ТРЕБУЕТСЯ нормализация, ибо дополнительные флаги меняют порядок */
    const details::tag_t normalized_tag = details::normalize_tag(
        details::make_tag(field->genus_and_id, false, false, false), false);

    while (span > 3) {
      const auto top = span;
      span >>= 1;
      const auto middle = iter + span;
      assert(middle->is_loose());
      if (middle->normalized_tag() < normalized_tag) {
        iter = middle + 1;
        span = top - span - 1;
      }
    }

    switch (span) {
    case 3:
      assert(iter->is_loose());
      if (iter->normalized_tag() >= normalized_tag)
        return (details::tag2genus_and_id(iter->tag()) == field->genus_and_id)
                   ? *iter
                   : token();
      ++iter;
      __fallthrough /* fall through */;
    case 2:
      assert(iter->is_loose());
      if (iter->normalized_tag() >= normalized_tag)
        return (details::tag2genus_and_id(iter->tag()) == field->genus_and_id)
                   ? *iter
                   : token();
      ++iter;
      __fallthrough /* fall through */;
    case 1:
      assert(iter->is_loose());
      if (iter->normalized_tag() >= normalized_tag)
        return (details::tag2genus_and_id(iter->tag()) == field->genus_and_id)
                   ? *iter
                   : token();
      ++iter;
      __fallthrough /* fall through */;
    case 0:
      break;
    default:
      assert(false);
      __unreachable();
    }
  }

  return token();
}

token schema::by_offset(const ptrdiff_t offset) const cxx11_noexcept {
  if (likely(offset >= 0 && offset < ptrdiff_t(preplaced_bytes()))) {
    const auto iter = search_preplaced(offset);
    const auto detent = tokens().begin() + number_of_preplaced();
    assert(iter >= tokens().begin() && iter <= detent);
    if (likely(iter < detent && offset == iter->preplaced_offset())) {
      assert(iter->is_preplaced());
      return *iter;
    }
  }
  return token();
}

token schema::next_by_offset(const ptrdiff_t offset) const cxx11_noexcept {
  if (likely(offset >= 0 && offset < ptrdiff_t(preplaced_bytes()))) {
    auto iter = search_preplaced(offset);
    const auto detent = tokens().begin() + number_of_preplaced();
    assert(iter >= tokens().begin() && iter <= detent);
    if (likely(iter < detent)) {
      assert(iter->preplaced_offset() >= offset);
      iter += iter->preplaced_offset() == offset;
      if (likely(iter < detent)) {
        assert(iter->is_preplaced());
        return *iter;
      }
    }
  }
  return token();
}

token schema::prev_by_offset(const ptrdiff_t offset) const cxx11_noexcept {
  if (likely(offset > 0 && offset <= ptrdiff_t(preplaced_bytes()))) {
    auto iter = search_preplaced(offset);
    assert(iter >= tokens().begin() &&
           iter <= tokens().begin() + number_of_preplaced());
    if (likely(--iter >= tokens().begin())) {
      assert(iter->preplaced_offset() < offset);
      return *iter;
    }
  }
  return token();
}

token schema::get_token(const string_view &field_name,
                        const boolean_option discernible_null,
                        const boolean_option saturated) const {
  const token ident =
      get_token_nothrow(field_name, discernible_null, saturated);
  if (unlikely(!ident.is_valid()))
    throw schema_no_such_field();
  return ident;
}

token schema::get_token(const token &inlay_token, const string_view &inner_name,
                        const boolean_option discernible_null,
                        const boolean_option saturated) const {
  const token ident =
      get_token_nothrow(inlay_token, inner_name, discernible_null, saturated);
  if (unlikely(!ident.is_valid()))
    throw schema_no_such_field();
  return ident;
}

string_view schema::get_name(const token &ident) const {
  const string_view name = get_name_nothrow(ident);
  if (unlikely(name.nil()))
    throw schema_no_such_field();
  return name;
}

std::size_t
schema::estimate_tuple_size(const token_vector &tokens,
                            std::size_t expected_average_stretchy_length) {
  std::size_t fixed_bytes =
      sizeof(details::stretchy_value_tuple /* tuple header */);
  std::size_t dynamic_units = 0;

  for (const token &ident : tokens) {
    if (ident.is_preplaced()) {
      fixed_bytes = std::max(fixed_bytes,
                             ident.preplaced_offset() + ident.preplaced_size());
      if (ident.type() == genus::hole)
        continue;
    } else {
      dynamic_units += 1 + details::loose_units_dynamic(ident.type());
    }
    if (ident.is_stretchy())
      dynamic_units += details::bytes2units(expected_average_stretchy_length);
  }

  return fixed_bytes + details::units2bytes(dynamic_units);
}

token schema::define_field(bool preplaced, std::string &&name, fptu::genus type,
                           const bool discernible_null, const bool saturation) {
  return preplaced ? define_preplaced(std::move(name), type, discernible_null,
                                      saturation)
                   : define_loose(std::move(name), type, false,
                                  discernible_null, saturation);
}

token schema::define_collection(std::string &&name, fptu::genus type,
                                const bool discernible_null,
                                const bool saturation) {
  return define_loose(std::move(name), type, true, discernible_null,
                      saturation);
}

} // namespace fptu
