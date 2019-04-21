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

#include "fast_positive/details/essentials.h"
#include "fast_positive/details/exceptions.h"
#include "fast_positive/details/meta.h"
#include "fast_positive/details/types.h"
#include "fast_positive/details/utils.h"

namespace fptu {
namespace details {
/* TODO */

template <genus FROM, genus TO> struct casting {
  using type_from = typename meta::genus_traits<FROM>::value_type;
  using type_to = typename meta::genus_traits<TO>::value_type;

  constexpr casting() {
    throw std::invalid_argument("fptu: field type mismatch");
  }
  constexpr const type_to &operator()(const type_from &value) {
    (void)value;
    return meta::genus_traits<TO>::denil;
  }
};

} // namespace details
} // namespace fptu
