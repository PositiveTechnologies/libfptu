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

#include "../fptu_test.h"
#include "fast_positive/details/cpu_features.h"

#include <array>
#include <vector>

#include "fast_positive/details/warnings_push_pt.h"

//------------------------------------------------------------------------------

#pragma pack(push, 1)
struct Foo {
  char x;
  int Bar;
};
#pragma pack(pop)

struct MyToken_FooBar_int : public FPTU_TOKEN(Foo, Bar) {
  MyToken_FooBar_int() noexcept {
    static_assert(static_offset == 1, "WTF?");
    static_assert(std::is_base_of<::fptu::details::token_static_tag,
                                  MyToken_FooBar_int>::value,
                  "WTF?");
    static_assert(MyToken_FooBar_int::is_static_token::value, "WTF?");
  }
};

FPTU_TEMPLATE_FOR_STATIC_TOKEN
bool probe2static(const TOKEN &token) {
  (void)token;
  return true;
}

bool probe2static(const fptu::token &token) {
  (void)token;
  return false;
}

TEST(Token, StaticPreplaced) {
  MyToken_FooBar_int token;

  EXPECT_TRUE(token.is_preplaced());
  EXPECT_FALSE(token.is_loose());
  EXPECT_FALSE(token.is_inlay());
  EXPECT_FALSE(token.is_collection());
  EXPECT_EQ(fptu::genus::i32, token.type());
  EXPECT_TRUE(probe2static(token));
  EXPECT_TRUE(MyToken_FooBar_int::is_static_preplaced);
  EXPECT_TRUE(MyToken_FooBar_int::is_static_token::value);

  const fptu::tuple_ro_weak tuple_ro;
  EXPECT_THROW(tuple_ro.collection(token).empty(), fptu::collection_required);
}

//------------------------------------------------------------------------------

static void test_ScanIndex(fptu::details::scan_func_t scan) {
  static_assert(sizeof(fptu::details::unit_t) ==
                    sizeof(fptu::details::field_loose),
                "Oops");
  std::array<fptu::details::unit_t, 64> array;
  memset(&array, 0, sizeof(array));
  fptu::details::field_loose *const fake =
      erthink::constexpr_pointer_cast<fptu::details::field_loose *>(&array);

  for (ptrdiff_t tail = array.size(); tail > 0; --tail) {
    if (tail != ptrdiff_t(array.size()))
      fake[tail].genius_and_id = 1;

    for (ptrdiff_t head = -1; head < tail; ++head) {
      if (head >= 0)
        fake[head].genius_and_id = 1;

      for (ptrdiff_t begin = array.size(); begin >= 0; --begin) {
        for (ptrdiff_t end = array.size(); end >= 0; --end) {
          auto ptr = scan(&fake[begin], &fake[end], 1);
          if (begin >= end) {
            ASSERT_EQ(nullptr, ptr);
          } else if (begin <= head && head < end) {
            ASSERT_EQ(&fake[head], ptr);
          } else if (end > tail && begin <= tail) {
            ASSERT_EQ(&fake[tail], ptr);
          } else {
            ASSERT_EQ(nullptr, ptr);
          }
        }
      }

      if (head >= 0)
        fake[head].genius_and_id = 0;
    }
    if (tail != ptrdiff_t(array.size()))
      fake[tail].genius_and_id = 0;
  }
}

TEST(ScanIndex, referential) {
  test_ScanIndex(fptu::details::fptu_scan_referential);
}

TEST(ScanIndex, unroll) { test_ScanIndex(fptu::details::fptu_scan_unroll); }

#ifdef __ia32__

TEST(ScanIndex, AVX2) {
  if (!fptu::cpu_features.has_AVX2())
    GTEST_SKIP();
  test_ScanIndex(fptu::details::fptu_scan_AVX2);
}

TEST(ScanIndex, AVX) {
  if (!fptu::cpu_features.has_AVX())
    GTEST_SKIP();
  test_ScanIndex(fptu::details::fptu_scan_AVX);
}

TEST(ScanIndex, SSE2) {
  if (!fptu::cpu_features.has_SSE2())
    GTEST_SKIP();
  test_ScanIndex(fptu::details::fptu_scan_SSE2);
}

#endif /* __ia32__ */

//------------------------------------------------------------------------------

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();

  fptu::tuple_ro_weak r0;
  fptu::tuple_ro_managed r1;

  fptu::tuple_rw_managed m0;
  fptu::tuple_rw_fixed f = fptu::tuple_rw_fixed::clone(m0);
  fptu::tuple_rw_managed m1 = fptu::tuple_rw_managed::clone(f);
  int x = (r0 == r1) + (m0 == r0) + (m0 == r1) + (m0 == f) + (m1 == m0);
  (void)x;
}

//------------------------------------------------------------------------------

template class fptu::tuple_crtp_reader<fptu::tuple_ro_weak>;
template class fptu::tuple_crtp_reader<fptu::tuple_ro_managed>;

#ifdef __GNUC__
#define INJECT_ASM(TEXT)
#else
#define INJECT_ASM(TEXT) (void)TEXT
#endif

__extern_C __dll_export int fptu2_get_Yint(const fptu::tuple_ro_weak tuple) {
  INJECT_ASM("#-stoken-preplaced--- get");
  int a = tuple.get_i32(MyToken_FooBar_int());
  INJECT_ASM("#-stoken-preplaced--- begin-loop");
  for (auto b : tuple.collection(MyToken_FooBar_int())) {
    INJECT_ASM("#-stoken-preplaced--- accessor.get");
    a += b.get_i32();
    INJECT_ASM("#-stoken-preplaced--- end-loop");
  }
  INJECT_ASM("#-stoken-preplaced--- is_present");
  return a + tuple.is_present(MyToken_FooBar_int());
  INJECT_ASM("#-stoken-preplaced--- end");
}

__extern_C __dll_export int fptu2_get_Xint(const fptu::tuple_ro_managed tuple,
                                           fptu::token token) {
  INJECT_ASM("#-dtoken-preplaced--- get");
  int a = tuple.get_i32(token);
  INJECT_ASM("#-dtoken-preplaced--- begin-loop");
  for (auto b : tuple.collection(token)) {
    INJECT_ASM("#-dtoken-preplaced--- accessor.get");
    a += b.get_i32();
    INJECT_ASM("#-dtoken-preplaced--- end-loop");
  }
  INJECT_ASM("#-dtoken-preplaced--- is_present");
  return a + tuple.is_present(token);
  INJECT_ASM("#-dtoken-preplaced--- get");
}

//------------------------------------------------------------------------------

template class fptu::tuple_crtp_writer<fptu::tuple_rw_fixed>;

__extern_C __dll_export int fptu2_rw_get_Yint(fptu::tuple_rw_fixed &tuple) {
  INJECT_ASM("#-stoken-preplaced--- get");
  int a = tuple.get_i32(MyToken_FooBar_int());
  INJECT_ASM("#-stoken-preplaced--- begin-loop");
  for (auto b : tuple.collection(MyToken_FooBar_int())) {
    INJECT_ASM("#-stoken-preplaced--- accessor.get");
    a += b.get_i32();
    INJECT_ASM("#-stoken-preplaced--- end-loop");
  }
  INJECT_ASM("#-stoken-preplaced--- is_present");
  return a + tuple.is_present(MyToken_FooBar_int());
  INJECT_ASM("#-stoken-preplaced--- end");
}

__extern_C __dll_export int fptu2_rw_get_Xint(fptu::tuple_rw_fixed &tuple,
                                              fptu::token token) {
  INJECT_ASM("#-dtoken-preplaced--- get");
  int a = tuple.get_i32(token);
  INJECT_ASM("#-dtoken-preplaced--- begin-loop");
  for (auto b : tuple.collection(token)) {
    INJECT_ASM("#-dtoken-preplaced--- accessor.get");
    a += b.get_i32();
    INJECT_ASM("#-dtoken-preplaced--- end-loop");
  }
  INJECT_ASM("#-dtoken-preplaced--- is_present");
  return a + tuple.is_present(token);
  INJECT_ASM("#-dtoken-preplaced--- get");
}
