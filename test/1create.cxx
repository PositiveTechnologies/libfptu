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
 */

#include "fptu_test.h"

TEST(Init, Invalid) {
  EXPECT_EQ(nullptr, fptu_init(nullptr, 0, 0));
  EXPECT_EQ(nullptr,
            fptu_init(nullptr, fptu_max_tuple_bytes / 2, fptu::max_fields / 2));
  EXPECT_EQ(nullptr,
            fptu_init(nullptr, fptu_max_tuple_bytes, fptu::max_fields));
  EXPECT_EQ(nullptr, fptu_init(nullptr, ~0u, ~0u));

  char space_exactly_noitems[fptu_rw::pure_tuple_size()];
  EXPECT_EQ(nullptr,
            fptu_init(space_exactly_noitems, sizeof(space_exactly_noitems), 1));
  EXPECT_EQ(nullptr,
            fptu_init(space_exactly_noitems, sizeof(space_exactly_noitems),
                      fptu::max_fields));
  EXPECT_EQ(nullptr, fptu_init(nullptr, sizeof(space_exactly_noitems), 0));
  EXPECT_NE(nullptr,
            fptu_init(space_exactly_noitems, sizeof(space_exactly_noitems), 0));
  EXPECT_EQ(nullptr, fptu_init(space_exactly_noitems,
                               sizeof(space_exactly_noitems) - 1, 0));
  EXPECT_EQ(nullptr, fptu_init(space_exactly_noitems, 0, 0));
  EXPECT_EQ(nullptr, fptu_init(space_exactly_noitems, 0, 1));
  EXPECT_EQ(nullptr, fptu_init(space_exactly_noitems, 0, fptu::max_fields));
  EXPECT_EQ(nullptr, fptu_init(space_exactly_noitems, 0, fptu::max_fields * 2));
  EXPECT_EQ(nullptr, fptu_init(space_exactly_noitems, 0, ~0u));

  char space_maximum[fptu::buffer_enough];
  EXPECT_EQ(nullptr, fptu_init(space_maximum, sizeof(space_maximum),
                               fptu::max_fields + 1));
  EXPECT_EQ(nullptr, fptu_init(nullptr, sizeof(space_maximum), 0));
  EXPECT_EQ(nullptr, fptu_init(space_exactly_noitems, ~0u, 1));
  ASSERT_EQ(nullptr, fptu_init(space_exactly_noitems, fptu_buffer_limit + 1,
                               fptu::max_fields));

  EXPECT_NE(nullptr, fptu_init(space_maximum, sizeof(space_maximum), 0));
  EXPECT_NE(nullptr, fptu_init(space_maximum, sizeof(space_maximum), 1));
  EXPECT_NE(nullptr, fptu_init(space_maximum, sizeof(space_maximum),
                               fptu::max_fields / 2));
  EXPECT_NE(nullptr,
            fptu_init(space_maximum, sizeof(space_maximum), fptu::max_fields));
}

TEST(Init, Base) {
  char space[fptu::buffer_enough];

  fptu_rw *pt0 = fptu_init(space, fptu_rw::pure_tuple_size(), 0);
  ASSERT_NE(nullptr, pt0);
  ASSERT_STREQ(nullptr, fptu::check(pt0));
  ASSERT_EQ(0u, fptu_space4items(pt0));
  ASSERT_EQ(0u, fptu_space4data(pt0));
  ASSERT_EQ(0u, fptu_junkspace(pt0));
  ASSERT_STREQ(nullptr, fptu::check(pt0));

  static const size_t extra_space_cases[] = {
      /* clang-format off */
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 42, fptu_rw::pure_tuple_size(),
        fptu_max_tuple_bytes / 3, fptu_max_tuple_bytes / 2,
        fptu_max_tuple_bytes
      /* clang-format on */
  };

  static const unsigned items_cases[] = {
      /* clang-format off */
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 42, ~0u, fptu::max_fields / 3,
        fptu::max_fields / 2, fptu::max_fields, fptu::max_fields + 1,
        fptu::max_fields * 2
      /* clang-format on */
  };

  for (auto extra : extra_space_cases) {
    size_t bytes = fptu_rw::pure_tuple_size() + extra;
    ASSERT_LE(bytes, sizeof(space));

    for (auto items : items_cases) {
      SCOPED_TRACE("extra " + std::to_string(extra) + ", items " +
                   std::to_string(items));

      fptu_rw *pt = fptu_init(space, bytes, items);
      if (items > extra / 4 || items > fptu::max_fields) {
        EXPECT_EQ(nullptr, pt);
        continue;
      }
      ASSERT_NE(nullptr, pt);

      fptu_ro io = fptu_take_noshrink(pt);
      EXPECT_NE(nullptr, io.units);
      EXPECT_EQ(size_t(fptu::unit_size), io.total_bytes);

      EXPECT_EQ(items, fptu_space4items(pt));
      size_t avail = std::min(size_t(fptu::max_tuple_bytes_netto),
                              FPT_ALIGN_FLOOR(extra, fptu::unit_size)) -
                     fptu::unit_size * items;
      EXPECT_EQ(avail, fptu_space4data(pt));
      EXPECT_EQ(0u, fptu_junkspace(pt));

      EXPECT_STREQ(nullptr, fptu_check_ro(io));
      EXPECT_STREQ(nullptr, fptu_check_rw(pt));
    }
  }
}

TEST(Init, Alloc) {
  fptu_rw *pt = fptu_alloc(7, 42);
  ASSERT_NE(nullptr, pt);
  ASSERT_STREQ(nullptr, fptu_check_rw(pt));
  fptu_destroy(pt);
}

//------------------------------------------------------------------------------

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
