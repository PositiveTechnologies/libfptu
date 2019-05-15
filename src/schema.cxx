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

#include "fast_positive/details/schema.h"
#include "fast_positive/details/api.h"
#include "fast_positive/details/bug.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/token.h"

#include <algorithm>
#include <unordered_map>

namespace fptu {

inline schema::schema() : stretchy_preplaced_(0) {}
schema::~schema() {}

namespace {
struct schema_impl : public schema {
  struct hash_name {
    bool operator()(const string_view &name) const noexcept {
      return name.hash_value();
    }
    bool operator()(const std::string &name) const noexcept {
      return operator()(string_view(name));
    }
  };

  //----------------------------------------------------------------------------

  using map_name2token = std::unordered_map<std::string, token>;
  using map_token2name =
      std::unordered_map<token, string_view, token::hash, token::same>;
  map_name2token name2token_;
  map_token2name token2name_;

  void purge() noexcept {
    sorted_tokens_.clear();
    stretchy_preplaced_ = 0;
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

  token get_token_nothrow(const string_view &field_name,
                          const boolean_option discernible_null,
                          const boolean_option saturated) const
      noexcept override;
  token get_token_nothrow(const token &inlay_token,
                          const string_view &inner_name,
                          const boolean_option discernible_null,
                          const boolean_option saturated) const
      noexcept override;
  string_view get_name_nothrow(const token &ident) const noexcept override;
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
  const token ident(fptu::genus(type + 1), 0, false, false, false);
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
      else
        preplaced_image_.append(ident.preplaced_size(), '\0');
      stretchy_preplaced_ += ident.is_stretchy();
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
} // namespace

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
  assert(utils::is_power2(align));
  const std::size_t unaligned_offset = preplaced_image_.size();
  const std::size_t aligned_offset =
      unaligned_offset + (align - unaligned_offset % align);
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

  assert(utils::is_power2(align));
  const std::size_t unaligned_offset = preplaced_image_.size();
  const std::size_t aligned_offset =
      unaligned_offset + (align - unaligned_offset % align);
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

token schema_impl::get_token_nothrow(const string_view &name,
                                     const boolean_option discernible_null,
                                     const boolean_option saturated) const
    noexcept {
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

token schema_impl::get_token_nothrow(const token &inlay_token,
                                     const string_view &inner_name,
                                     const boolean_option discernible_null,
                                     const boolean_option saturated) const
    noexcept {
  (void)inlay_token;
  (void)inner_name;
  (void)discernible_null;
  (void)saturated;
  FPTU_NOT_IMPLEMENTED();
  return token();
}

string_view schema_impl::get_name_nothrow(const token &ident) const noexcept {
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
