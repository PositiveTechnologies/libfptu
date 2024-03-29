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

#include "fptu_test.h"

#include <cmath>

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#pragma warning(push, 1)
#include <windows.h>
#pragma warning(pop)

#pragma warning(disable : 4244) /* 'initializing' : conversion from 'double'   \
                                   to 'uint32_t', possible loss of data */
#pragma warning(disable : 4365) /* 'initializing' : conversion from 'int' to   \
                                   'uint32_t', signed / unsigned mismatch */

static void usleep(unsigned usec) {
  HANDLE timer;
  LARGE_INTEGER li;

  /* Convert to 100 nanosecond interval,
   * negative value indicates relative time */
  li.QuadPart = -(10 * (int64_t)usec);

  timer = CreateWaitableTimer(NULL, TRUE, NULL);
  SetWaitableTimer(timer, &li, 0, NULL, NULL, 0);
  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
}
#endif /* Windows */

const auto ms100 = fptu::datetime_t::ms2fractional(100);

TEST(Trivia, Denil) {
  union {
    double f;
    uint64_t u;
  } denil64;
  denil64 = {FPTU_DENIL_FP64};
  EXPECT_EQ(FPTU_DENIL_FP64_BIN, denil64.u);
  denil64 = {fptu_fp64_denil()};
  EXPECT_EQ(FPTU_DENIL_FP64_BIN, denil64.u);
  denil64 = {fptu_fp32_denil()};
  EXPECT_NE(FPTU_DENIL_FP64_BIN, denil64.u);

  union {
    float f;
    uint32_t u;
  } denil32;
  denil32 = {FPTU_DENIL_FP32};
  EXPECT_EQ(FPTU_DENIL_FP32_BIN, denil32.u);
  denil32 = {fptu_fp32_denil()};
  EXPECT_EQ(FPTU_DENIL_FP32_BIN, denil32.u);
  denil32 = {static_cast<float>(fptu_fp64_denil())};
  EXPECT_EQ(FPTU_DENIL_FP32_BIN, denil32.u);
}

TEST(Trivia, Apriory) {
  ASSERT_EQ(UINT16_MAX, (uint16_t)-1ll);
  ASSERT_EQ(UINT32_MAX, (uint32_t)-1ll);
  ASSERT_TRUE(FPT_IS_POWER2(fptu::unit_size));
  ASSERT_EQ(fptu::unit_size, 1 << fptu_unit_shift);

  ASSERT_LE(std::ptrdiff_t(fptu_max_cols), std::ptrdiff_t(fptu::max_fields));
  ASSERT_LE(fptu_max_field_bytes, UINT16_MAX * fptu::unit_size);
  ASSERT_LE(std::ptrdiff_t(fptu_max_opaque_bytes),
            std::ptrdiff_t(fptu_max_field_bytes) -
                std::ptrdiff_t(fptu::unit_size));

  ASSERT_GE(fptu_max_field_bytes, fptu::max_fields * fptu::unit_size);
  ASSERT_GE(fptu_max_tuple_bytes, fptu_max_field_bytes + fptu::unit_size * 2);
  ASSERT_GE(fptu_max_tuple_bytes, (fptu::max_fields + 1) * fptu::unit_size * 2);
  ASSERT_LE(fptu::buffer_enough, fptu::buffer_limit);

#define HERE_TEST_ITEM(GENUS)                                                  \
  ASSERT_FALSE(fptu::details::is_fixed_size(                                   \
      fptu::details::make_tag(fptu::genus::GENUS, 0, false, true, false)));    \
  ASSERT_FALSE(fptu::details::is_fixed_size(                                   \
      fptu::details::make_tag(fptu::genus::GENUS, 0, true, true, false)))
  HERE_TEST_ITEM(text);
  HERE_TEST_ITEM(varbin);
  HERE_TEST_ITEM(nested);
  HERE_TEST_ITEM(property);
#undef HERE_TEST_ITEM

#define HERE_TEST_ITEM(N)                                                      \
  ASSERT_TRUE(fptu::details::is_fixed_size(                                    \
      fptu::details::make_tag(fptu::genus(N), 0, false, true, false)));        \
  ASSERT_TRUE(fptu::details::is_fixed_size(                                    \
      fptu::details::make_tag(fptu::genus(N), 0, true, true, false)))
  HERE_TEST_ITEM(4);
  HERE_TEST_ITEM(5);
  HERE_TEST_ITEM(6);
  HERE_TEST_ITEM(7);
  HERE_TEST_ITEM(8);
  HERE_TEST_ITEM(9);
  HERE_TEST_ITEM(10);
  HERE_TEST_ITEM(11);
  HERE_TEST_ITEM(12);
  HERE_TEST_ITEM(13);
  HERE_TEST_ITEM(14);
  HERE_TEST_ITEM(15);
  HERE_TEST_ITEM(16);
  HERE_TEST_ITEM(17);
  HERE_TEST_ITEM(18);
  HERE_TEST_ITEM(19);
  HERE_TEST_ITEM(20);
  HERE_TEST_ITEM(21);
  HERE_TEST_ITEM(22);
  HERE_TEST_ITEM(23);
  HERE_TEST_ITEM(24);
  HERE_TEST_ITEM(25);
  HERE_TEST_ITEM(26);
  HERE_TEST_ITEM(27);
  HERE_TEST_ITEM(28);
  HERE_TEST_ITEM(29);
  HERE_TEST_ITEM(30);
#undef HERE_TEST_ITEM
  ASSERT_EQ(31, int(fptu::genus::hole));

  ASSERT_EQ(0u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_uint16)));
  ASSERT_EQ(0u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_16)));

  ASSERT_EQ(1u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_int32)));
  ASSERT_EQ(1u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_uint32)));
  ASSERT_EQ(1u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_fp32)));
  ASSERT_EQ(1u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_32)));

  ASSERT_EQ(2u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_int64)));
  ASSERT_EQ(2u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_uint64)));
  ASSERT_EQ(2u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_fp64)));
  ASSERT_EQ(2u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_64)));

  ASSERT_EQ(4u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_128)));
  ASSERT_EQ(5u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_160)));
  ASSERT_EQ(2u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_datetime)));
  ASSERT_EQ(8u, fptu::details::loose_units_dynamic(
                    fptu::details::tag2genus(fptu_256)));

  ASSERT_EQ(sizeof(struct iovec), sizeof(fptu_ro));

  ASSERT_EQ(fptu_rw::pure_tuple_size(), fptu_space(0, 0));
}

TEST(Trivia, ColType) {
  fptu_tag_t tag;
  tag = fptu::make_tag(0, fptu_bool);
  /* fpu_tag_t is uint_fast 16_t, it can be 2, 4, or 8 bytes in size */
  ASSERT_EQ((sizeof(tag) == 2) ? 0x2000u : 0xFFFD2000u, tag);
  EXPECT_EQ(0u, fptu::get_colnum(tag));
  EXPECT_EQ(fptu_bool, fptu::get_type(tag));

  tag = fptu::make_tag(42, fptu_int64);
  ASSERT_NE(0u, tag);
  EXPECT_EQ(42u, fptu::get_colnum(tag));
  EXPECT_EQ(fptu_int64, fptu::get_type(tag));
}

TEST(Trivia, cmp2int) {
  EXPECT_EQ(0, fptu::cmp2int(41, 41));
  EXPECT_EQ(1, fptu::cmp2int(42, 41));
  EXPECT_EQ(-1, fptu::cmp2int(41, 42));

  EXPECT_EQ(0, fptu::cmp2int(-41, -41));
  EXPECT_EQ(1, fptu::cmp2int(0, -41));
  EXPECT_EQ(-1, fptu::cmp2int(-41, 0));

  EXPECT_EQ(1, fptu::cmp2int(42, -42));
  EXPECT_EQ(-1, fptu::cmp2int(-42, 42));
}

TEST(Trivia, cmp2lge) {
  EXPECT_EQ(fptu_eq, fptu::cmp2lge(41, 41));
  EXPECT_EQ(fptu_gt, fptu::cmp2lge(42, 41));
  EXPECT_EQ(fptu_lt, fptu::cmp2lge(41, 42));

  EXPECT_EQ(fptu_eq, fptu::cmp2lge(-41, -41));
  EXPECT_EQ(fptu_gt, fptu::cmp2lge(0, -41));
  EXPECT_EQ(fptu_lt, fptu::cmp2lge(-41, 0));

  EXPECT_EQ(fptu_gt, fptu::cmp2lge(42, -42));
  EXPECT_EQ(fptu_lt, fptu::cmp2lge(-42, 42));
}

TEST(Trivia, diff2lge) {
  EXPECT_EQ(fptu_eq, fptu::diff2lge(0));
  EXPECT_EQ(fptu_gt, fptu::diff2lge(1));
  EXPECT_EQ(fptu_gt, fptu::diff2lge(INT_MAX));
  EXPECT_EQ(fptu_gt, fptu::diff2lge(LONG_MAX));
  EXPECT_EQ(fptu_gt, fptu::diff2lge(ULONG_MAX));
  EXPECT_EQ(fptu_lt, fptu::diff2lge(-1));
  EXPECT_EQ(fptu_lt, fptu::diff2lge(INT_MIN));
  EXPECT_EQ(fptu_lt, fptu::diff2lge(LONG_MIN));
}

TEST(Trivia, iovec) {
  ASSERT_EQ(sizeof(struct iovec), sizeof(fptu_ro));

  fptu_ro serialized;
  serialized.sys.iov_len = 42;
  serialized.sys.iov_base = &serialized;

  ASSERT_EQ(&serialized.total_bytes, &serialized.sys.iov_len);
  ASSERT_EQ(sizeof(serialized.total_bytes), sizeof(serialized.sys.iov_len));
  ASSERT_EQ(serialized.total_bytes, serialized.sys.iov_len);

  ASSERT_EQ((void *)&serialized.units, &serialized.sys.iov_base);
  ASSERT_EQ(sizeof(serialized.units), sizeof(serialized.sys.iov_base));
  ASSERT_EQ(serialized.units, serialized.sys.iov_base);
}

//------------------------------------------------------------------------------

TEST(Trivia, time_ns2fractional) {
  const double scale = exp2(32) / 1e9;
  for (int base_2log = 0; base_2log < 32; ++base_2log) {
    for (int offset_42 = -42; offset_42 <= 42; ++offset_42) {
      SCOPED_TRACE("base_2log " + std::to_string(base_2log) + ", offset_42 " +
                   std::to_string(offset_42));
      const uint64_t ns = (uint64_t(1) << base_2log) + offset_42;
      if (ns >= 1000000000)
        continue;
      SCOPED_TRACE("ns " + std::to_string(ns) + ", factional " +
                   std::to_string(ns * scale));
      const uint64_t probe = floor(ns * scale);
      ASSERT_EQ(probe, fptu::datetime_t::ns2fractional(ns));
    }
  }
}

TEST(Trivia, time_fractional2ns) {
  const double scale = 1e9 / exp2(32);
  for (int base_2log = 0; base_2log < 32; ++base_2log) {
    for (int offset_42 = -42; offset_42 <= 42; ++offset_42) {
      SCOPED_TRACE("base_2log " + std::to_string(base_2log) + ", offset_42 " +
                   std::to_string(offset_42));
      const uint64_t fractional =
          uint32_t((uint64_t(1) << base_2log) + offset_42);
      SCOPED_TRACE("fractional " + std::to_string(fractional) + ", ns " +
                   std::to_string(fractional * scale));
      const uint64_t probe = floor(fractional * scale);
      ASSERT_EQ(probe, fptu::datetime_t::fractional2ns(fractional));
    }
  }
}

TEST(Trivia, time_us2fractional) {
  const double scale = exp2(32) / 1e6;
  for (int base_2log = 0; base_2log < 32; ++base_2log) {
    for (int offset_42 = -42; offset_42 <= 42; ++offset_42) {
      SCOPED_TRACE("base_2log " + std::to_string(base_2log) + ", offset_42 " +
                   std::to_string(offset_42));
      const uint64_t us = (uint64_t(1) << base_2log) + offset_42;
      if (us >= 1000000)
        continue;
      SCOPED_TRACE("us " + std::to_string(us) + ", factional " +
                   std::to_string(us * scale));
      const uint64_t probe = floor(us * scale);
      ASSERT_EQ(probe, fptu::datetime_t::us2fractional(us));
    }
  }
}

TEST(Trivia, time_fractional2us) {
  const double scale = 1e6 / exp2(32);
  for (int base_2log = 0; base_2log < 32; ++base_2log) {
    for (int offset_42 = -42; offset_42 <= 42; ++offset_42) {
      SCOPED_TRACE("base_2log " + std::to_string(base_2log) + ", offset_42 " +
                   std::to_string(offset_42));
      const uint64_t fractional =
          uint32_t((uint64_t(1) << base_2log) + offset_42);
      SCOPED_TRACE("fractional " + std::to_string(fractional) + ", us " +
                   std::to_string(fractional * scale));
      const uint64_t probe = floor(fractional * scale);
      ASSERT_EQ(probe, fptu::datetime_t::fractional2us(fractional));
    }
  }
}

TEST(Trivia, time_ms2fractional) {
  const double scale = exp2(32) / 1e3;
  for (int base_2log = 0; base_2log < 32; ++base_2log) {
    for (int offset_42 = -42; offset_42 <= 42; ++offset_42) {
      SCOPED_TRACE("base_2log " + std::to_string(base_2log) + ", offset_42 " +
                   std::to_string(offset_42));
      const uint64_t ms = (uint64_t(1) << base_2log) + offset_42;
      if (ms >= 1000)
        continue;
      SCOPED_TRACE("ms " + std::to_string(ms) + ", factional " +
                   std::to_string(ms * scale));
      const uint64_t probe = floor(ms * scale);
      ASSERT_EQ(probe, fptu::datetime_t::ms2fractional(ms));
    }
  }
}

TEST(Trivia, time_fractional2ms) {
  const double scale = 1e3 / exp2(32);
  for (int base_2log = 0; base_2log < 32; ++base_2log) {
    for (int offset_42 = -42; offset_42 <= 42; ++offset_42) {
      SCOPED_TRACE("base_2log " + std::to_string(base_2log) + ", offset_42 " +
                   std::to_string(offset_42));
      const uint64_t fractional =
          uint32_t((uint64_t(1) << base_2log) + offset_42);
      SCOPED_TRACE("fractional " + std::to_string(fractional) + ", ms " +
                   std::to_string(fractional * scale));
      const uint64_t probe = floor(fractional * scale);
      ASSERT_EQ(probe, fptu::datetime_t::fractional2ms(fractional));
    }
  }
}

TEST(Trivia, time_coarse) {
  auto prev = fptu_now_coarse();
  for (auto n = 0; n < 42; ++n) {
    auto now = fptu_now_coarse();
    ASSERT_GE(now.fixedpoint, prev.fixedpoint);
    prev = now;
    usleep(137);
  }
}

TEST(Trivia, time_fine) {
  auto prev = fptu_now_fine();
  for (auto n = 0; n < 42; ++n) {
    auto now = fptu_now_fine();
    ASSERT_GE(now.fixedpoint, prev.fixedpoint);
    prev = now;
    usleep(137);
  }
}

TEST(Trivia, time_coarse_vs_fine) {
  for (auto n = 0; n < 42; ++n) {
    auto coarse = fptu_now_coarse();
    auto fine = fptu_now_fine();
    ASSERT_GE(fine.fixedpoint, coarse.fixedpoint);
    ASSERT_GT(ms100, fine.fixedpoint - coarse.fixedpoint);
    usleep(137);
  }
}

namespace std {
template <typename T> std::string to_hex(const T &v) {
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << v;
  return stream.str();
}
} // namespace std

TEST(Trivia, time_grain) {
  for (int grain = -32; grain < 0; ++grain) {
    SCOPED_TRACE("grain " + std::to_string(grain));

    auto prev = fptu_now(grain);
    for (auto n = 0; n < 42; ++n) {
      auto grained = fptu_now(grain);
      ASSERT_GE(grained.fixedpoint, prev.fixedpoint);
      prev = grained;
      auto fine = fptu_now_fine();
      SCOPED_TRACE("grained.hex " + std::to_hex(grained.fractional) +
                   ", fine.hex " + std::to_hex(fine.fractional));
      ASSERT_GE(fine.fixedpoint, grained.fixedpoint);
      for (int bit = 0; - bit > grain; ++bit) {
        SCOPED_TRACE("bit " + std::to_string(bit));
        EXPECT_EQ(0u, grained.fractional & (uint64_t(1) << bit));
      }
      usleep(37);
    }
  }
}

//------------------------------------------------------------------------------

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
