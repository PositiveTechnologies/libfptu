/*
 *  Fast Positive Tuples (libfptu), aka Позитивные Кортежи
 *  Copyright 2016-2019 Leonid Yuriev <leo@yuriev.ru>
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

#include "fast_positive/details/api.h"

#include "fast_positive/details/1Hippeus/hipagut.h"
#include "fast_positive/details/bug.h"
#include "fast_positive/details/erthink/erthink_intrin.h"
#include "fast_positive/details/erthink/erthink_mul.h"

#include <time.h>
static __always_inline std::size_t ticks() {
  /* TODO: use libmera */
  uint64_t u64;

#if defined(EMSCRIPTEN)
  u64 = emscripten_get_now() * 1e6;
#elif defined(__e2k__) || defined(__elbrus__)
  u64 = __rdtsc();
#elif defined(__ia32__)
  u64 = __rdtsc();
#elif defined(__APPLE__) || defined(__MACH__)
  u64 = mach_absolute_time();
#elif defined(__WINDOWS__)
  QueryPerformanceCounter((LARGE_INTEGER *)&u64);
#elif defined(CLOCK_MONOTONIC)
  struct timespec ts;
  clock_gettime(posix_clockid, &ts);
  u64 = ts.tv_nsec | ts.tv_sec << 32;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  u64 = tv.tv_usec | tv.tv_sec << 32;
#endif

  return static_cast<size_t>(u64 ^ (u64 >> 32));
}

//-----------------------------------------------------------------------------

using namespace erthink;

/* Calculate magic mesh from the ticks and constant signature */
static __always_inline uint32_t mixup(size_t ticks, const uint32_t signature) {
  // multiplication by a prime number.
  uint64_t mesh =
      mul_32x32_64(UINT32_C(0xD0C3DFD7), uint32_t(ticks + signature));
  // significally less collisions when only the few bits damaged.
  return uint32_t(mesh ^ (mesh >> 32));
}

/* Checks that the value is sufficiently different from 0 and all ones */
static __always_inline bool fairly(uint32_t value) {
  return value < UINT32_C(0xFFFF0000) && value > UINT32_C(0x0000FFFF);
}

__hot __flatten void hipagut_setup(hippeus_hipagut *gizmo,
                                   const uint32_t signature) {
  uint32_t chirp = uint32_t(ticks());
  while (true) {
    if (likely(fairly(chirp))) {
      uint32_t mesh = mixup(chirp, signature);
      if (likely(fairly(mesh))) {
        gizmo->random_chirp = chirp;
        gizmo->derived_mesh = mesh;
        return;
      }
    }
    chirp = chirp * UINT32_C(1664525) + UINT32_C(1013904223);
  }
}

unsigned hipagut_nasty_disabled;

__hot __flatten bool hipagut_probe(const hippeus_hipagut *gizmo,
                                   const uint32_t signature) {
  if (likely(hipagut_nasty_disabled != HIPPEUS_HIPAGUT_NASTY_DISABLED)) {
    const hippeus_hipagut snapshot = *gizmo;
    if (unlikely(!fairly(snapshot.random_chirp)))
      return false;
    if (unlikely(!fairly(snapshot.derived_mesh)))
      return false;
    if (unlikely(snapshot.derived_mesh !=
                 mixup(snapshot.random_chirp, signature)))
      return false;
  }
  return true;
}
