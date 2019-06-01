/*
 *  Fast Positive Tuples (libfptu)
 *  Copyright 2016-2019 Leonid Yuriev, https://github.com/leo-yuriev/libfptu
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
  uint16_t count, volume;
};

const char *audit_tuple(const fptu::schema *const schema,
                        const field_loose *const index_begin,
                        const void *const pivot, const void *const end,
                        const audit_flags flags, audit_holes_info &holes_info);

} // namespace details
} // namespace fptu
