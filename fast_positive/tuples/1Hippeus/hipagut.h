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

#include "fast_positive/tuples/details/warnings_push_system.h"

#include <cstddef>
#include <cstdint>

#include "fast_positive/tuples/details/warnings_pop.h"

#include "fast_positive/tuples/details/warnings_push_pt.h"

/* `Hipagut` -- a sister in law, nurse (Tagalog language, Philippines).
 * Simple signature-based memory checking engine. */
#ifndef HIPAGUT_DISABLED
#define HIPAGUT_DISABLED 0
#endif

#if HIPAGUT_DISABLED

#define HIPPEUS_ASSERT_CHECK 0
#define HIPAGUT_N42(label) 0
#define HIPAGUT_DECLARE(gizmo) /* nope */
#define HIPAGUT_SPACE 0

#define HIPAGUT_SETUP(gizmo, label) __noop()
#define HIPAGUT_DROWN(gizmo) __noop()

#define HIPAGUT_PROBE(gizmo, label) true
#define HIPAGUT_SETUP_LINK(slave, master) __noop()

#define HIPAGUT_PROBE_LINK(slave, master) true
#define HIPAGUT_SETUP_ASIDE(base, label, offset) __noop()

#define HIPAGUT_DROWN_ASIDE(base, offset) __noop()
#define HIPAGUT_PROBE_ASIDE(base, label, offset) true

#define HIPAGUT_SETUP_LINK_ASIDE(base, master, offset) __noop()
#define HIPAGUT_PROBE_LINK_ASIDE(base, master, offset) true

#define HIPAGUT_ENSURE(gizmo, label) __noop()
#define HIPAGUT_ENSURE_ASIDE(base, label, offset) __noop()
#define HIPAGUT_ENSURE_LINK(slave, master) __noop()
#define HIPAGUT_ENSURE_LINK_ASIDE(base, master, offset) __noop()

#else

typedef union hippeus_hipagut hippeus_hipagut_t;

union FPTU_API_TYPE hippeus_hipagut {
  struct {
    volatile uint32_t random_chirp, derived_mesh;
  };
  volatile uint64_t body;
};

#ifdef __cplusplus
cxx14_constexpr uint32_t operator"" _hipagut(const char *str, std::size_t len) {
  uint32_t h = __LINE__;
  for (std::size_t i = 0; i < len; ++i)
    h = str[i] ^ (h * UINT32_C(1664525) + UINT32_C(1013904223));
  return h;
}
#define HIPAGUT_N42(label) #label##_hipagut
#endif /* __cplusplus */

__extern_C void hipagut_setup(hippeus_hipagut_t *, const uint32_t signature);
__extern_C __must_check_result bool hipagut_probe(const hippeus_hipagut_t *,
                                                  const uint32_t signature);
static __maybe_unused __always_inline void
hipagut_setup_link(hippeus_hipagut_t *slave, const hippeus_hipagut_t *master) {
  hipagut_setup(slave, master->derived_mesh);
}

static __maybe_unused __always_inline __must_check_result bool
hipagut_probe_link(const hippeus_hipagut *slave,
                   const hippeus_hipagut *master) {
  constexpr_assert("42"_hipagut != 0);
  return hipagut_probe(slave, master->derived_mesh);
}

#define HIPPEUS_HIPAGUT_NASTY_DISABLED 0xfea51b1eu /* feasible */
extern unsigned hipagut_nasty_disabled;

static __maybe_unused __always_inline void
hipagut_drown(hippeus_hipagut *gizmo) {
  // LY: This notable value would always bite,
  //     because (random_chirp == 0) != (derived_hash == 0).
  gizmo->body = UINT32_C(0xDEADB0D1F);
}

//-----------------------------------------------------------------------------

#define HIPAGUT_DECLARE(gizmo) hippeus_hipagut_t gizmo

#define HIPAGUT_SPACE ((ptrdiff_t)sizeof(hippeus_hipagut_t))

#define HIPAGUT_SETUP(gizmo, label) hipagut_setup(&(gizmo), HIPAGUT_N42(label))

#define HIPAGUT_DROWN(gizmo) hipagut_drown(&(gizmo))

#define HIPAGUT_PROBE(gizmo, label) hipagut_probe(&(gizmo), HIPAGUT_N42(label))

#define HIPAGUT_SETUP_LINK(slave, master)                                      \
  hipagut_setup_link(&(slave), &(master))

#define HIPAGUT_PROBE_LINK(slave, master)                                      \
  hipagut_probe_link(&(slave), &(master))

//-----------------------------------------------------------------------------

#define HIPAGUT_ASIDE(base, offset)                                            \
  ((hippeus_hipagut *)(((char *)(base)) + (offset)))

#define HIPAGUT_BEFORE(address) HIPAGUT_ASIDE(address, -HIPAGUT_SPACE)

//-----------------------------------------------------------------------------

#define HIPAGUT_SETUP_ASIDE(base, label, offset)                               \
  hipagut_setup(HIPAGUT_ASIDE(base, offset), HIPAGUT_N42(label))

#define HIPAGUT_DROWN_ASIDE(base, offset)                                      \
  hipagut_drown(HIPAGUT_ASIDE(base, offset))

#define HIPAGUT_PROBE_ASIDE(base, label, offset)                               \
  hipagut_probe(HIPAGUT_ASIDE(base, offset), HIPAGUT_N42(label))

#define HIPAGUT_SETUP_LINK_ASIDE(base, master, offset)                         \
  hipagut_setup_link(HIPAGUT_ASIDE(base, offset), &(master))

#define HIPAGUT_PROBE_LINK_ASIDE(base, master, offset)                         \
  hipagut_probe_link(HIPAGUT_ASIDE(base, offset), &(master))

//-----------------------------------------------------------------------------

#define HIPAGUT_ENSURE(gizmo, label)                                           \
  do                                                                           \
    if (unlikely(!HIPAGUT_PROBE(gizmo, label)))                                \
      FPTU_RAISE_BUG(__LINE__, #label "@" #gizmo, __func__, __FILE__);         \
  while (0)

#define HIPAGUT_ENSURE_ASIDE(base, label, offset)                              \
  do                                                                           \
    if (unlikely(!HIPAGUT_PROBE_ASIDE(base, label, offset)))                   \
      FPTU_RAISE_BUG(__LINE__, #label "@" #base "[" #offset "]", __func__,     \
                     __FILE__);                                                \
  while (0)

#define HIPAGUT_ENSURE_LINK(slave, master)                                     \
  do                                                                           \
    if (unlikely(!HIPAGUT_PROBE_LINK(slave, master)))                          \
      FPTU_RAISE_BUG(__LINE__, #master "->" #slave, __func__, __FILE__);       \
  while (0)

#define HIPAGUT_ENSURE_LINK_ASIDE(base, master, offset)                        \
  do                                                                           \
    if (unlikely(!HIPAGUT_PROBE_LINK_ASIDE(base, master, offset)))             \
      FPTU_RAISE_BUG(__LINE__, #master "->" #base "[" #offset "]", __func__,   \
                     __FILE__);                                                \
  while (0)
#endif /* !HIPAGUT_DISABLED */

//-----------------------------------------------------------------------------

#if HIPPEUS_ASSERT_CHECK
#define HIPAGUT_ASSERT(gizmo, label) HIPAGUT_ENSURE(gizmo, label)
#define HIPAGUT_ASSERT_ASIDE(base, label, offset)                              \
  HIPAGUT_ENSURE_ASIDE(base, label, offset)
#define HIPAGUT_ASSERT_LINK(slave, maser) HIPAGUT_ENSURE_LINK(slave, master)
#define HIPAGUT_ASSERT_LINK_ASIDE(base, master, offset)                        \
  HIPAGUT_ENSURE_LINK_ASIDE(base, master, offset)
#else
#define HIPAGUT_ASSERT(gizmo, label) __noop()
#define HIPAGUT_ASSERT_ASIDE(base, label, offset) __noop()
#define HIPAGUT_ASSERT_LINK(slave, master) __noop()
#define HIPAGUT_ASSERT_LINK_ASIDE(base, master, offset) __noop()
#endif

#include "fast_positive/tuples/details/warnings_pop.h"
