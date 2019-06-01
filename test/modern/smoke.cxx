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
  EXPECT_TRUE(MyToken_FooBar_int::is_static_preplaced());
  EXPECT_TRUE(MyToken_FooBar_int::is_static_token::value);

#ifndef __clang__
  const fptu::tuple_ro_weak tuple_ro;
  /* FIXME: CLANG ?-6-7-8 WTF? */
  EXPECT_THROW(tuple_ro.collection(token).empty(), ::fptu::collection_required);
#endif
}

#ifdef __clang__
TEST(clang_WTF, DISABLED_ExceptionHandling) {
#else
TEST(clang_WTF, ExceptionHandling) {
#endif
  try {
    bool got_collection_required_exception = false;
    try {
      // fptu::throw_collection_required();
      const fptu::tuple_ro_weak tuple_ro;
      MyToken_FooBar_int token;
      tuple_ro.collection(token).empty();
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

//------------------------------------------------------------------------------

TEST(Smoke, trivia_set) {
  fptu::tuple_rw_managed rw;
  fptu::token token(fptu::u16, 0);
  rw.set_u16(token, 42);
  auto value = rw.get_u16(token);
  EXPECT_EQ(42, value);
}

TEST(Smoke, trivia_autogrowth) {
  fptu::tuple_rw_managed rw;
  fptu::token token(fptu::text, 0, true);
  EXPECT_GT(size_t(fptu::max_tuple_bytes_netto), rw.capacity());
  for (int i = 1; i < 555; ++i)
    rw.insert_string(token,
                     fptu::format("This is the string #%*d.", i - 555, i));

  EXPECT_EQ(size_t(fptu::max_tuple_bytes_netto), rw.capacity());
}

TEST(Smoke, autogrowth_with_preplaced) {
  auto schema = fptu::schema::create();

  std::vector<std::pair<std::string, std::size_t>> values = {
      {"event_src.host", 16},
      {"event_src.hostname", 16},
      {"event_src.subsys", 8},
      {"event_src.title", 7},
      {"event_src.vendor", 9}, /*{"generator", 9},*/
      {"id", 53},
      {"mime", 25},
      {"msgid", 4},
      {"object.id", 1037}};

  for (auto p : values)
    schema->define_loose(std::move(p.first), fptu::genus::text);

  schema->define_preplaced("generator", fptu::genus::text);
  fptu::defaults::setup(fptu::initiation_scale::small, std::move(schema));

  fptu::tuple_rw_managed rw;
  for (auto p : values) {
    std::string stub{};
    stub.resize(p.second, 'a');
    rw.set_string(fptu::defaults::schema->get_token(p.first.data()), stub);
  }
  rw.take_managed_clone_optimized();
}

TEST(Smoke, trivia_managing) {
  cxx14_constexpr fptu::token token_utc32(fptu::genus::datetime_utc, 0);
  cxx14_constexpr fptu::token token_datetime64(fptu::genus::datetime_utc, 0);
  cxx14_constexpr fptu::token token_i64(fptu::i64, 0);
  fptu::tuple_rw_fixed rw_fixed;
  rw_fixed.set_datetime(token_utc32, fptu::datetime_t::now());
  rw_fixed.set_datetime(token_utc32, fptu::datetime_t::now_coarse());
  rw_fixed.set_datetime(token_datetime64, fptu::datetime_t::now_fine());
  rw_fixed.set_integer(token_i64, INT64_MIN);
  rw_fixed.set_integer(token_i64, INT64_MAX);

  fptu::tuple_ro_weak ro_weak = rw_fixed.take_weak().first;
  EXPECT_FALSE(ro_weak.empty());
  EXPECT_TRUE(ro_weak.is_present(token_utc32));
  EXPECT_TRUE(ro_weak.is_present(token_datetime64));
  EXPECT_TRUE(ro_weak.is_present(token_i64));

  fptu::tuple_ro_managed ro_managed = rw_fixed.take_managed_clone().first;
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  ro_managed = rw_fixed.move_to_ro();
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  rw_fixed = std::move(ro_managed);
  ro_managed = rw_fixed.take_managed_clone().first;
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  rw_fixed = fptu::tuple_rw_fixed::clone(ro_managed);
  ro_weak = rw_fixed.take_weak().first;
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  rw_fixed = fptu::tuple_rw_fixed::clone(ro_weak);
  ro_weak = rw_fixed.take_weak().first;
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  EXPECT_EQ(1, ro_managed.get_buffer()->ref_counter);
  auto ro_managed2 = ro_managed;
  EXPECT_EQ(2, ro_managed.get_buffer()->ref_counter);
  ro_managed.purge();
  EXPECT_FALSE(ro_managed);
  EXPECT_EQ(1, ro_managed2.get_buffer()->ref_counter);
}

//------------------------------------------------------------------------------

TEST(Smoke, trivia_schema_definition) {
  auto schema = fptu::schema::create();
  for (unsigned n = 0; n < 42; ++n)
    for (fptu::genus type = fptu::genus(0); type != fptu::genus::hole;
         type = fptu::genus(type + 1))
      schema->define_field(
          false, fptu::format("#%u of %s", n, std::to_string(type).c_str()),
          type);
  std::set<std::string> names = {"datafield1",   "event_src.host",
                                 "event_src.ip", "event_src.title",
                                 "generator",    "id",
                                 "mime",         "object.name",
                                 "reason",       "subject.group",
                                 "subject.id",   "tag",
                                 "type"};
  for (auto item : names)
    schema->define_field(item == "generator", std::move(item),
                         fptu::genus::text);
  fptu::defaults::setup(fptu::initiation_scale::small, std::move(schema));

  fptu::tuple_rw_managed rw;
  rw.set_string(fptu::defaults::schema->get_token("datafield1"),
                std::string("229099411"));
  rw.set_string(fptu::defaults::schema->get_token("event_src.host"),
                std::string("91.142.135.113"));
  rw.set_string(fptu::defaults::schema->get_token("event_src.ip"),
                std::string("91.142.135.113"));
  rw.set_string(fptu::defaults::schema->get_token("event_src.title"),
                std::string("unix_like"));
  rw.set_string(fptu::defaults::schema->get_token("generator"),
                std::string("N8.0.1309"));
  rw.set_string(fptu::defaults::schema->get_token("id"),
                std::string("PT_UNIX_like_auditd_syslog_path_msg"));
  rw.set_string(fptu::defaults::schema->get_token("mime"),
                std::string("text/plain"));
  rw.set_string(fptu::defaults::schema->get_token("object.name"),
                std::string("/proc/1/comm"));
  rw.set_string(fptu::defaults::schema->get_token("reason"),
                std::string("File was created or deleted"));
  rw.set_string(fptu::defaults::schema->get_token("subject.group"),
                std::string("0"));
  rw.set_string(fptu::defaults::schema->get_token("subject.id"),
                std::string("0"));
  rw.set_string(fptu::defaults::schema->get_token("tag"),
                std::string("syslog"));
  rw.set_string(fptu::defaults::schema->get_token("type"), std::string("norm"));
  rw.take_managed_clone_optimized();
}

//------------------------------------------------------------------------------

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
