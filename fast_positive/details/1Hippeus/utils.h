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
 *
 * ***************************************************************************
 *
 * Imported (with simplification) from 1Hippeus project.
 * Copyright (c) 2006-2013 Leonid Yuriev <leo@yuriev.ru>.
 */

#pragma once
#include "fast_positive/details/api.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
namespace hippeus {

static constexpr uint64_t linear_congruential(uint64_t value) {
  return value * 6364136223846793005ull + 1442695040888963407ull;
}

static constexpr uint32_t linear_congruential(uint32_t value) {
  return value * 1664525u + 1013904223u;
}

extern const std::size_t pagesize;
void probe_pages(const void *addr, ptrdiff_t bytes, bool rw,
                 std::size_t pagesize = hippeus::pagesize);
__must_check_result int
prefault_oomsafe(const void *addr, ptrdiff_t bytes,
                 std::size_t pagesize = hippeus::pagesize);
void pollute(void *ptr, ptrdiff_t bytes, uintptr_t xormask = 0);
bool is_zeroed(const void *ptr, ptrdiff_t bytes);
size_t log2size(size_t value, unsigned log2min, unsigned log2max,
                unsigned log2step);
} // namespace hippeus
#endif /* __cplusplus */
