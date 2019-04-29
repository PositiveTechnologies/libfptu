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

#pragma once
#include "fast_positive/details/api.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/string_view.h"
#include "fast_positive/details/token.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "fast_positive/details/warnings_push_pt.h"

namespace fptu {
namespace details {

#pragma pack(push, 1)
#pragma pack(pop)

} // namespace details

class FPTU_API_TYPE schema {
public:
  using token_vector = std::vector<token>;

private:
  unsigned preplaced_bytes_;
  const char *preplaced_init_image_;

  token_vector stored_tokens_;
  std::string stored_dict_;
  std::unordered_map<string_view, token> map_;

public:
  const token_vector &tokens() const { return stored_tokens_; }
  token bind(const string_view &field_name, const bool quietabsence = false,
             const bool saturated = false);
  token bind(const token &inlay_token, const string_view &field_name,
             const bool quietabsence = false, const bool saturated = false);
  std::size_t preplaced_bytes() const noexcept { return preplaced_bytes_; }
  std::size_t preplaced_units() const noexcept {
    return details::bytes2units(preplaced_bytes());
  }
  const void *preplaced_init_image() const { return preplaced_init_image_; }
  std::size_t stretchy_preplaced() const {
    return 42 /* TODO: заранее посчитать и выдавать количество таких полей */;
  }

  schema() : preplaced_bytes_(0), preplaced_init_image_(nullptr) {}
  virtual ~schema() { delete[] preplaced_init_image_; }
};

} // namespace fptu

#include "fast_positive/details/warnings_pop.h"
