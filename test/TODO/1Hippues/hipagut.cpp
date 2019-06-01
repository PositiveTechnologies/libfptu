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
 *
 * ***************************************************************************
 *
 * Imported (with simplification) from 1Hippeus project.
 * Copyright (c) 2006-2013 Leonid Yuriev <leo@yuriev.ru>.
 */

#include <gtest/gtest.h>
#include <hippeus/hipagut.h>
#include <hippeus/hipagut_allocator.h>

#include <stdlib.h>
#include <sys/prctl.h>

#include "../emul.h"
#include "../primes.h"

#include <map>
#include <string>

using namespace hippeus;

static unsigned nonzero_random() {
  unsigned number = 0;
  while (!number)
    number = mrand48();

  return number;
}

TEST(Hipagut, Setup) {
  HIPAGUT_DECLARE(gizmo);
  HIPAGUT_SETUP(gizmo, test);
  EXPECT_TRUE(HIPAGUT_PROBE(gizmo, test));

  EXPECT_FALSE(hipagut_probe(&gizmo, 0));
  EXPECT_FALSE(hipagut_probe(&gizmo, ~0));
  EXPECT_FALSE(hipagut_probe(&gizmo, 1));
  EXPECT_FALSE(hipagut_probe(&gizmo, ~1));

  hipagut_setup(&gizmo, 0);
  EXPECT_TRUE(hipagut_probe(&gizmo, 0));

  hipagut_setup(&gizmo, ~0);
  EXPECT_TRUE(hipagut_probe(&gizmo, ~0));

  hipagut_setup(&gizmo, 1);
  EXPECT_TRUE(hipagut_probe(&gizmo, 1));

  hipagut_setup(&gizmo, ~1);
  EXPECT_TRUE(hipagut_probe(&gizmo, ~1));

  for (int i = 0; i < 1111; ++i) {
    unsigned number = nonzero_random();
    hipagut_setup(&gizmo, number);
    EXPECT_TRUE(hipagut_probe(&gizmo, number));
  }
}

TEST(Hipagut, Breakdown) {
  HIPAGUT_DECLARE(gizmo);
  HIPAGUT_DROWN(gizmo);

  EXPECT_FALSE(HIPAGUT_PROBE(gizmo, test));
  EXPECT_FALSE(hipagut_probe(&gizmo, 0));
  EXPECT_FALSE(hipagut_probe(&gizmo, ~0));
  EXPECT_FALSE(hipagut_probe(&gizmo, 1));
  EXPECT_FALSE(hipagut_probe(&gizmo, ~1));

  for (int i = 0; i < 1111; ++i) {
    unsigned number = nonzero_random();
    EXPECT_FALSE(hipagut_probe(&gizmo, number));
  }
}

//-----------------------------------------------------------------------------

struct Therein {
  HIPAGUT_DECLARE(guard);
};

TEST(Hipagut, Therein) {
  struct Therein *holder = new struct Therein();

  HIPAGUT_SETUP(holder->guard, test);
  EXPECT_TRUE(HIPAGUT_PROBE(holder->guard, test));
  EXPECT_FALSE(hipagut_probe(&holder->guard, 0));

  hipagut_setup(&holder->guard, 0);
  EXPECT_TRUE(hipagut_probe(&holder->guard, 0));
  EXPECT_FALSE(HIPAGUT_PROBE(holder->guard, test));

  HIPAGUT_DROWN(holder->guard);
  EXPECT_FALSE(HIPAGUT_PROBE(holder->guard, test));
  EXPECT_FALSE(hipagut_probe(&holder->guard, 0));

  delete holder;
}

//-----------------------------------------------------------------------------

TEST(Hipagut, Relative) {
  unsigned const size = 42;
  char *holder = new char[size + HIPAGUT_SPACE];

  HIPAGUT_SETUP_ASIDE(holder, test, size);
  EXPECT_TRUE(HIPAGUT_PROBE_ASIDE(holder, test, size));
  EXPECT_FALSE(hipagut_probe(HIPAGUT_ASIDE(holder, size), 0));

  memset(holder, 0, size);
  EXPECT_TRUE(HIPAGUT_PROBE_ASIDE(holder, test, size));
  EXPECT_FALSE(hipagut_probe(HIPAGUT_ASIDE(holder, size), 0));

  memset(holder, 0, size + HIPAGUT_SPACE);
  EXPECT_FALSE(HIPAGUT_PROBE_ASIDE(holder, test, size));
  EXPECT_FALSE(hipagut_probe(HIPAGUT_ASIDE(holder, size), 0));

  memset(holder, ~0, size + HIPAGUT_SPACE);
  EXPECT_FALSE(HIPAGUT_PROBE_ASIDE(holder, test, size));
  EXPECT_FALSE(hipagut_probe(HIPAGUT_ASIDE(holder, size), 0));

  hipagut_setup(HIPAGUT_ASIDE(holder, size), 0);
  EXPECT_TRUE(hipagut_probe(HIPAGUT_ASIDE(holder, size), 0));
  EXPECT_FALSE(HIPAGUT_PROBE_ASIDE(holder, test, size));

  HIPAGUT_DROWN_ASIDE(holder, size);
  EXPECT_FALSE(HIPAGUT_PROBE_ASIDE(holder, test, size));
  EXPECT_FALSE(hipagut_probe(HIPAGUT_ASIDE(holder, size), 0));

  delete[] holder;
}

//-----------------------------------------------------------------------------

TEST(Hipagut, Link) {
  HIPAGUT_DECLARE(master);
  HIPAGUT_SETUP(master, true);
  EXPECT_TRUE(HIPAGUT_PROBE(master, true));

  HIPAGUT_DECLARE(slave);
  HIPAGUT_SETUP_LINK(slave, master);
  EXPECT_TRUE(HIPAGUT_PROBE_LINK(slave, master));

  unsigned const size = 42;
  char *holder = new char[size + HIPAGUT_SPACE];
  HIPAGUT_SETUP_LINK_ASIDE(holder, master, size);
  EXPECT_TRUE(HIPAGUT_PROBE_LINK_ASIDE(holder, master, size));

  HIPAGUT_DROWN(slave);
  EXPECT_FALSE(HIPAGUT_PROBE_LINK(slave, master));
  HIPAGUT_SETUP_LINK(slave, master);
  EXPECT_TRUE(HIPAGUT_PROBE_LINK(slave, master));

  HIPAGUT_DROWN_ASIDE(holder, size);
  EXPECT_FALSE(HIPAGUT_PROBE_LINK_ASIDE(holder, master, size));
  HIPAGUT_SETUP_LINK_ASIDE(holder, master, size);
  EXPECT_TRUE(HIPAGUT_PROBE_LINK_ASIDE(holder, master, size));

  HIPAGUT_DROWN(master);
  EXPECT_FALSE(HIPAGUT_PROBE_LINK(slave, master));
  EXPECT_FALSE(HIPAGUT_PROBE_LINK_ASIDE(holder, master, size));

  HIPAGUT_DROWN(slave);
  EXPECT_FALSE(HIPAGUT_PROBE_LINK(slave, master));

  HIPAGUT_DROWN_ASIDE(holder, size);
  EXPECT_FALSE(HIPAGUT_PROBE_LINK_ASIDE(holder, master, size));

  delete[] holder;
}

//-----------------------------------------------------------------------------

static void bite(hip_hipagut_t *gizmo, const uint64_t origin) {
  static uint64_t salt = 2305843009213693967ull;
  uint64_t mask, prev = gizmo->tale;

  do {
    unsigned number = mrand48();

    switch ((number >> 6) & 3) {
    default:
      // LY: sector of 0xFF.
      mask = (~0ull) << (number & 070);
      mask = rotl(mask, (number >> 8) & 070);
      break;
    case 2:
      // LY: shift by 0/8/16/..56 bits = make few 0xFF on the right side.
      mask = (~0ull) >> (number & 070);
      break;
    case 3:
      // LY: shift by 0/8/16/..56 bits = make few 0xFF on the left side.
      mask = (~0ull) << (number & 070);
      break;
    }

    switch (number & 7) {
    default:
      // damage by random bytes
      gizmo->tale = origin ^ (mask & salt);
      break;
    case 0 ... 1:
      // damage by 0x00
      gizmo->tale = origin & ~mask;
      break;
    case 2 ... 3:
      // damage by 0xFF
      gizmo->tale = origin | mask;
      break;
    }

    salt = number + salt * 4824586234576574581ull;
  } while (origin == gizmo->tale || prev == gizmo->tale);

  //::printf("0x%016lx => 0x%016lx, mask 0x%016lx, diff 0x%016lx/%u\n",
  //    origin, gizmo->tale, mask, origin ^ gizmo->tale,
  //    __builtin_popcount(origin ^ gizmo->tale));
}

TEST(Hipagut, Bite) {
  static struct {
    uint64_t iters, pops;
    unsigned slips, hard;
  } total;

  unsigned slips = 0;

  HIPAGUT_DECLARE(gizmo);

  for (int i = 0; i < 1111; ++i) {
    unsigned number = nonzero_random();

    hipagut_setup(&gizmo, number);
    EXPECT_TRUE(hipagut_probe(&gizmo, number));

    EXPECT_FALSE(hipagut_probe(&gizmo, 0) && number != 0);
    EXPECT_FALSE(hipagut_probe(&gizmo, ~0) && number != ~0u);
    EXPECT_FALSE(hipagut_probe(&gizmo, 1) && number != 1);
    EXPECT_FALSE(hipagut_probe(&gizmo, ~1) && number != ~1u);

    const uint64_t origin = (gizmo).tale;
    for (int j = 0; j < 1111; ++j) {
      bite(&(gizmo), origin);
      uint64_t diff = gizmo.tale ^ origin;
      unsigned weight = hamming_weight(diff);
      total.pops += weight;
      total.iters++;
      if (hipagut_probe(&gizmo, number)) {
        total.slips++;
        total.hard += hi32(diff) == 0 || lo32(diff) == 0;
        slips++;
        fprintf(stderr,
                "SLIP: %08x, %016lx~%016lx, diff %08lx.%08lx:%-2u, ratio %lE "
                "%u-%u/%lu, %6.3lf %lu/%lu\n",
                number, origin, gizmo.tale, diff >> 32, diff & 0xFFFFFFFFu,
                weight, total.slips / (double)total.iters, total.slips,
                total.hard, total.iters, total.pops / (double)total.iters,
                total.pops, total.iters);
        EXPECT_TRUE(hi32(diff) == 0 || lo32(diff) == 0);
      }
    }

    HIPAGUT_DROWN(gizmo);
    EXPECT_FALSE(hipagut_probe(&gizmo, number));
  }

  ASSERT_TRUE(slips <
              3 /* LY: failure probability < 1e-23 (1e-10^3 * 1111^2) */);
}

//-----------------------------------------------------------------------------

static void probe_hipagut_allocator(int bite) {
  std::vector<char, hipagut_std_allocator<char>> victim(1);
  auto ptr = victim.data();
  ptr[bite]++;
}

TEST(CheckedAllocator, Normal) { ASSERT_NO_THROW(probe_hipagut_allocator(0)); }

TEST(CheckedAllocator, DeathUnder) {
#if HIPPEUS_ASSERT_CHECK
  EXPECT_EXIT(probe_hipagut_allocator(-1), testing::KilledBySignal(SIGABRT),
              ": Assertion `hipagut-guard");
#else
  hippeus_ensure_termination_disabled = HIPPEUS_ENSURE_TERMINATION_DISABLED;
  // EXPECT_EXIT(probe_hipagut_allocator(-1), testing::KilledBySignal(SIGABRT),
  // "1Hippeus caught the EXCEPTION/BUG");
  ASSERT_ANY_THROW(probe_hipagut_allocator(-1));
  hippeus_ensure_termination_disabled = 0;
#endif
}

TEST(CheckedAllocator, DeathOver) {
#if HIPPEUS_ASSERT_CHECK
  EXPECT_EXIT(probe_hipagut_allocator(1), testing::KilledBySignal(SIGABRT),
              ": Assertion `hipagut-guard");
#else
  hippeus_ensure_termination_disabled = HIPPEUS_ENSURE_TERMINATION_DISABLED;
  // EXPECT_EXIT(probe_hipagut_allocator(1), testing::KilledBySignal(SIGABRT),
  // "1Hippeus caught the EXCEPTION/BUG");
  ASSERT_ANY_THROW(probe_hipagut_allocator(1));
  hippeus_ensure_termination_disabled = 0;
#endif
}

typedef std::basic_string<char, std::char_traits<char>,
                          hipagut_std_allocator<char>>
    check_string;
typedef std::map<
    check_string, check_string, std::less<check_string>,
    hipagut_std_allocator<std::pair<const check_string, check_string>>>
    check_map;

static void StochasticsGame() {
  hipagut_std_allocator<check_map> allocator;
  check_map *map = allocator.allocate(1);
  allocator.construct(map);

  for (int n = 0; n < 11111; ++n) {
    check_string key = std::to_string(rand() % 1111).c_str();
    if (rand() % 3)
      map->operator[](key) = check_string(rand() % 111, '*');
    else
      map->erase(key);
  }
  map->clear();

  allocator.deallocate(map, 1);
}

TEST(CheckedAllocator, StochasticsGame) {
  hipagut_std_allocator<check_map> nanny_allocator;
  std::allocator<check_map> std_allocator;
  ASSERT_LT(nanny_allocator.max_size(), std_allocator.max_size());
  ASSERT_NO_THROW(StochasticsGame());
}

//-----------------------------------------------------------------------------

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::FLAGS_gtest_death_test_style = "threadsafe";

  // disable core dumps (speedup ~500 ms)
  prctl(PR_SET_DUMPABLE, 0);

  return RUN_ALL_TESTS();
}
