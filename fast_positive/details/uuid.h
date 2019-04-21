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

#include "fast_positive/details/fixed_binary.h"
#include <stdint.h>

typedef struct fptu_uuid {
  fptu_binary128_t bin128;
#ifdef __cplusplus
#endif
} fptu_uuid_t;

#ifdef __cplusplus
namespace fptu {
typedef ::fptu_uuid_t uuid_t;
} // namespace fptu
#endif /* __cplusplus */
