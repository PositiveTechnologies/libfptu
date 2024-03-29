/*
 *  Copyright (c) 1994-2021 Leonid Yuriev <leo@yuriev.ru>.
 *  https://github.com/erthink/erthink
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

#include "testing.h++"

#include "erthink_defs.h"
#include "erthink_u2a.h++"

#if defined(__LCC__)
#pragma diag_suppress 186 /* pointless comparison of unsigned integer with     \
                             zero [-Wtype-limits] */
#endif

__hot __dll_export __noinline char *_dec2(const unsigned value, char *ptr) {
  return erthink::dec2(value, ptr);
}

__hot __dll_export __noinline char *_dec3(const unsigned value, char *ptr) {
  return erthink::dec3(value, ptr);
}

__hot __dll_export __noinline char *_dec4(const unsigned value, char *ptr) {
  return erthink::dec4(value, ptr);
}

__hot __dll_export __noinline char *_u2a(const uint32_t value, char *ptr) {
  return erthink::u2a(value, ptr);
}

__hot __dll_export __noinline char *_u2a(const uint64_t value, char *ptr) {
  return erthink::u2a(value, ptr);
}

__hot __dll_export __noinline char *_i2a(const int32_t value, char *ptr) {
  return erthink::i2a(value, ptr);
}

__hot __dll_export __noinline char *_i2a(const int64_t value, char *ptr) {
  return erthink::i2a(value, ptr);
}

//------------------------------------------------------------------------------

TEST(u2a, dec2) {
  char buffer[8];
  for (unsigned i = 0; i < 100; ++i) {
    char *u2a_end = _dec2(i, buffer);
    ASSERT_LT(buffer, u2a_end);
    ASSERT_GT(erthink::array_end(buffer), u2a_end);
    *u2a_end = '\0';

    char *strtol_end = nullptr;
    long probe = strtol(buffer, &strtol_end, 10);
    EXPECT_EQ(u2a_end, strtol_end);
    EXPECT_EQ((long)i, probe);
  }
}

TEST(u2a, dec3) {
  char buffer[8];
  for (unsigned i = 0; i < 1000; ++i) {
    char *u2a_end = _dec3(i, buffer);
    ASSERT_LT(buffer, u2a_end);
    ASSERT_GT(erthink::array_end(buffer), u2a_end);
    *u2a_end = '\0';

    char *strtol_end = nullptr;
    long probe = strtol(buffer, &strtol_end, 10);
    EXPECT_EQ(u2a_end, strtol_end);
    EXPECT_EQ((long)i, probe);
  }
}

TEST(u2a, dec4) {
  char buffer[8];
  for (unsigned i = 0; i < 10000; ++i) {
    char *u2a_end = _dec4(i, buffer);
    ASSERT_LT(buffer, u2a_end);
    ASSERT_GT(erthink::array_end(buffer), u2a_end);
    *u2a_end = '\0';

    char *strtol_end = nullptr;
    long probe = strtol(buffer, &strtol_end, 10);
    EXPECT_EQ(u2a_end, strtol_end);
    EXPECT_EQ((long)i, probe);
  }
}

//------------------------------------------------------------------------------

template <typename T, ptrdiff_t MAX>
void probe(const T value, char *(*func)(T, char *)) {
  char buffer[MAX + 1];
  char *u2a_end = func(value, buffer);
  ASSERT_LT(buffer, u2a_end);
  ASSERT_GT(erthink::array_end(buffer), u2a_end);
  EXPECT_LE(u2a_end - buffer, MAX);
  *u2a_end = '\0';

  char *strtol_end = nullptr;
  T probe = static_cast<T>((value < 0) ? strtoll(buffer, &strtol_end, 10)
                                       : strtoull(buffer, &strtol_end, 10));
  EXPECT_EQ(u2a_end, strtol_end);
  EXPECT_EQ(value, probe);
}

template <typename T, unsigned MAX>
void probe_runbit(const T value, char *(*func)(T, char *)) {
  probe<T, MAX>(value, func);
  probe<T, MAX>(~value, func);
  for (unsigned i = 0; i < CHAR_BIT * sizeof(T); ++i) {
    T one = T(1) << i;
    probe<T, MAX>(one ^ value, func);
    probe<T, MAX>(one ^ ~value, func);
  }
}

TEST(u2a, uint32_to_a) {
  uint32_t v = UINT32_MAX;
  do {
    probe_runbit<uint32_t, 11>(v, _u2a);
    v >>= 1;
  } while (v);
  probe_runbit<uint32_t, 11>(0, _u2a);
}

TEST(u2a, int32_to_a) {
  probe_runbit<int32_t, 12>(INT32_MIN, _i2a);
  int32_t v = INT32_MAX;
  do {
    probe_runbit<int32_t, 12>(v, _i2a);
    v >>= 1;
  } while (v);
  probe_runbit<int32_t, 12>(0, _i2a);
}

TEST(u2a, uint64_to_a) {
  uint64_t v = UINT64_MAX;
  do {
    probe_runbit<uint64_t, 20>(v, _u2a);
    v >>= 1;
  } while (v);
  probe_runbit<uint64_t, 20>(0, _u2a);
}

TEST(u2a, int64_to_a) {
  probe_runbit<int64_t, 20>(INT64_MIN, _i2a);
  int64_t v = INT64_MAX;
  do {
    probe_runbit<int64_t, 20>(v, _i2a);
    v >>= 1;
  } while (v);
  probe_runbit<int64_t, 20>(0, _i2a);
}

TEST(u2a, random3e6) {
  uint64_t prng = uint64_t(time(0));
  SCOPED_TRACE("PRNG seed" + std::to_string(prng));
  for (int i = 0; i < 3000000; ++i) {
    probe<uint64_t, 20>(prng, _u2a);
    probe<int64_t, 20>(int64_t(~prng), _i2a);
    probe<uint32_t, 10>(static_cast<uint32_t>(prng >> 17), _u2a);
    probe<int32_t, 11>(static_cast<int32_t>(prng >> 23), _i2a);
    prng *= UINT64_C(6364136223846793005);
    prng += UINT64_C(1442695040888963407);
  }
}

//------------------------------------------------------------------------------

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
