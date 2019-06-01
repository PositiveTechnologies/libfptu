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
#include <hippeus/ensure.h>

#include <stdlib.h>
#include <sys/prctl.h>

TEST(Ensure, DeathByAssertion) {
  ASSERT_NO_THROW(HIPPEUS_ASSERT(true));
#if HIPPEUS_ASSERT_CHECK
  EXPECT_EXIT(HIPPEUS_ASSERT(false), ::testing ::KilledBySignal(SIGABRT),
              ": Assertion `false' failed");
#endif
}

TEST(Ensure, DeathByException) {
  ASSERT_NO_THROW(HIPPEUS_ENSURE(true));
#if HIPPEUS_ASSERT_CHECK
  EXPECT_EXIT(HIPPEUS_ENSURE(false), ::testing ::KilledBySignal(SIGABRT),
              ": Assertion `false' failed");
#else
  hippeus_ensure_termination_disabled = HIPPEUS_ENSURE_TERMINATION_DISABLED;
  // EXPECT_EXIT(HIPPEUS_BUG(false), ::testing ::KilledBySignal(SIGABRT),
  // "1Hippeus caught the EXCEPTION/BUG");
  ASSERT_ANY_THROW(HIPPEUS_BUG(false));
  hippeus_ensure_termination_disabled = 0;
#endif
}

TEST(Ensure, DeathByBug) {
#if HIPPEUS_ASSERT_CHECK
  EXPECT_EXIT(HIPPEUS_BUG(false), ::testing ::KilledBySignal(SIGABRT),
              ": Assertion `false' failed");
#else
  hippeus_ensure_termination_disabled = HIPPEUS_ENSURE_TERMINATION_DISABLED;
  // EXPECT_EXIT(HIPPEUS_BUG(false), ::testing ::KilledBySignal(SIGABRT),
  // "1Hippeus caught the EXCEPTION/BUG");
  ASSERT_ANY_THROW(HIPPEUS_BUG(false));
  hippeus_ensure_termination_disabled = 0;
#endif
}

static void dummy_function() {}

TEST(Ensure, is_executable) {
  EXPECT_TRUE(hippeus_is_executable((const void *)&dummy_function));
  EXPECT_FALSE(hippeus_is_executable(nullptr));
}

static const bool dummy_readable = false;

TEST(Ensure, is_readable) {
  EXPECT_TRUE(hippeus_is_readable(&dummy_readable));
  EXPECT_TRUE(hippeus_is_readable((const void *)&dummy_function));
  EXPECT_FALSE(hippeus_is_readable(nullptr));
}

static bool dummy_writeable;

TEST(Ensure, is_writeable) {
  EXPECT_TRUE(hippeus_is_writeable(&dummy_writeable));
  EXPECT_FALSE(hippeus_is_writeable(nullptr));
  // TODO
  // EXPECT_FALSE(hippeus_is_writeable(&dummy_readable));
  // EXPECT_FALSE(hippeus_is_writeable((const void*) &dummy_function));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";

  // disable core dumps (speedup ~500 ms)
  ::prctl(PR_SET_DUMPABLE, 0);

  return RUN_ALL_TESTS();
}
