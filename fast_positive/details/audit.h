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

#include "fast_positive/details/api.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/schema.h"

namespace fptu {
namespace details {

enum audit_flags {
  audit_default = 0,
  audit_tuple_sorted_loose = 1,
  audit_tuple_have_preplaced = 2,
  audit_adjacent_holes = 4,
};
DEFINE_ENUM_FLAG_OPERATORS(audit_flags)

struct audit_holes_info {
  uint16_t holes_count, holes_volume;
};

const char *audit_tuple(const fptu::schema *const schema,
                        const field_loose *const index_begin,
                        const void *const pivot, const void *const end,
                        const audit_flags flags, audit_holes_info &holes_info);

} // namespace details
} // namespace fptu
