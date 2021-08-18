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
 */

#include "../fptu_test.h"
#include "fast_positive/erthink/erthink_misc.h"
#include "fast_positive/tuples/details/cpu_features.h"

#include <array>
#include <vector>

#include "fast_positive/tuples/details/warnings_push_pt.h"

//------------------------------------------------------------------------------

typedef void (*throwable_function)();

static __noinline void probe(throwable_function fn) {
  try {
    bool got_collection_required_exception = false;
    try {
      fn();
    } catch (const ::fptu::collection_required &) {
      got_collection_required_exception = true;
    }
    EXPECT_TRUE(got_collection_required_exception);
  } catch (const ::std::exception &e) {
    std::string msg = fptu::format("Unexpected exception type '%s': %s",
                                   typeid(e).name(), e.what());
    GTEST_FATAL_FAILURE_(msg.c_str());
  } catch (...) {
    GTEST_FATAL_FAILURE_("Unknown NOT std::exception");
  }
}

static __noinline void throw_insideDSO() { throw fptu::collection_required(); }

static __noinline void throw_direct_crossoverDSO() {
  fptu::throw_collection_required();
}

static __noinline void throw_indirect_crossoverDSO() {
  const fptu::tuple_ro_weak tuple_ro;
  fptu::token token(fptu::u16, 0, false, false, false);
  tuple_ro.collection(token).empty();
}

TEST(ExceptionHandling, InsideDSO) { probe(throw_insideDSO); }

TEST(ExceptionHandling, DirectCrossoverDSO) {
  probe(throw_direct_crossoverDSO);
}

TEST(ExceptionHandling, IndirectCrossoverDSO) {
  probe(throw_indirect_crossoverDSO);
}

//------------------------------------------------------------------------------

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
