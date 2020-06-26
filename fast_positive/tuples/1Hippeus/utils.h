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
 *
 * ***************************************************************************
 *
 * Imported (with simplification) from 1Hippeus project.
 * Copyright (c) 2006-2013 Leonid Yuriev <leo@yuriev.ru>.
 */

#pragma once
#include "fast_positive/tuples/api.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
namespace hippeus {

static cxx11_constexpr uint64_t linear_congruential(uint64_t value) {
  return value * 6364136223846793005ull + 1442695040888963407ull;
}

static cxx11_constexpr uint32_t linear_congruential(uint32_t value) {
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
