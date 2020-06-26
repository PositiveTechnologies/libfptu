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

#pragma once
#include "fast_positive/tuples/api.h"
#include <exception>    // for str::exception
#include <new>          // for std::bad_alloc
#include <stdexcept>    // for std::invalid_argument
#include <system_error> // for std::system_error

namespace fptu {
class bug_location;
class schema;
namespace details {
class tuple_ro;
class tuple_rw;
} // namespace details

#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif

FPTU_API __noreturn void throw_tuple_bad(const fptu::schema *schema,
                                         const void *const ptr,
                                         const std::size_t bytes,
                                         const char *details);
FPTU_API __noreturn void throw_tuple_bad(const fptu::schema *schema,
                                         const details::tuple_ro *tuple);
FPTU_API __noreturn void throw_tuple_bad(const fptu::schema *schema,
                                         const details::tuple_ro *tuple,
                                         const char *details);
FPTU_API __noreturn void throw_tuple_bad(const details::tuple_rw *tuple);
FPTU_API __noreturn void throw_tuple_bad(const details::tuple_rw *tuple,
                                         const char *details);

FPTU_API __noreturn void throw_tuple_hollow();
FPTU_API __noreturn void throw_invalid_argument();
FPTU_API __noreturn void throw_invalid_argument(const char *details);
FPTU_API __noreturn void throw_invalid_allot();
FPTU_API __noreturn void throw_invalid_allot(const char *details);
FPTU_API __noreturn void throw_invalid_schema();
FPTU_API __noreturn void throw_invalid_schema(const char *details);
FPTU_API __noreturn void throw_type_mismatch();
FPTU_API __noreturn void throw_index_corrupted();
FPTU_API __noreturn void throw_field_absent();
FPTU_API __noreturn void throw_collection_unallowed();
FPTU_API __noreturn void throw_collection_required();
FPTU_API __noreturn void throw_value_denil();
FPTU_API __noreturn void throw_value_range();
FPTU_API __noreturn void throw_value_too_long();
FPTU_API __noreturn void throw_tuple_too_large();
FPTU_API __noreturn void throw_insufficient_space(size_t index,
                                                  std::size_t data);
FPTU_API __noreturn void throw_tuple_overflow();
FPTU_API __noreturn void throw_schema_mismatch();
FPTU_API __noreturn void throw_schema_mismatch(const char *details);
FPTU_API __noreturn void throw_schema_definition_error(const char *details);

} // namespace fptu

//------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4275) /* non dll-interface class 'FOO' used as base  \
                                   for dll-interface class */
#endif

namespace fptu /* __dll_visibility_default */ {

class FPTU_API_TYPE bug : public std::runtime_error {
  const bug_location &location_;

public:
  bug(const bug_location &) cxx11_noexcept;
  const bug_location &location() const cxx11_noexcept { return location_; }
  virtual ~bug() cxx11_noexcept;
};

class FPTU_API_TYPE bad_tuple : public std::runtime_error {
  using base = std::runtime_error;

public:
  bad_tuple(const char *details);
  bad_tuple(const std::string &details);
  virtual ~bad_tuple() cxx11_noexcept;
};

class FPTU_API_TYPE bad_tuple_ro : public bad_tuple {
  const fptu::schema *const schema_;
  const void *const ptr_;
  const std::size_t bytes_;

public:
  bad_tuple_ro(const fptu::schema *schema, const void *const ptr,
               const std::size_t bytes, const char *details);
  bad_tuple_ro(const fptu::schema *schema, const details::tuple_ro *tuple,
               const char *details);
  bad_tuple_ro(const fptu::schema *schema, const details::tuple_ro *tuple,
               const std::string &details);
  bad_tuple_ro(const fptu::schema *schema, const details::tuple_ro *tuple);
  const fptu::schema *schema() const cxx11_noexcept { return schema_; }
  const void *ptr() const cxx11_noexcept { return ptr_; }
  std::size_t bytes() const cxx11_noexcept { return bytes_; }
  virtual ~bad_tuple_ro() cxx11_noexcept;
};

class FPTU_API_TYPE bad_tuple_rw : public bad_tuple {
  const details::tuple_rw *const tuple_;

public:
  bad_tuple_rw(const details::tuple_rw *tuple, const char *details);
  bad_tuple_rw(const details::tuple_rw *tuple, const std::string &details);
  bad_tuple_rw(const details::tuple_rw *tuple);
  const details::tuple_rw *tuple() const cxx11_noexcept { return tuple_; }
  virtual ~bad_tuple_rw() cxx11_noexcept;
};

//------------------------------------------------------------------------------

class FPTU_API_TYPE insufficient_space : public std::bad_alloc {
public:
  const std::size_t index_space, data_space;
  insufficient_space(size_t index_space, std::size_t data_space) cxx11_noexcept;
  virtual ~insufficient_space() cxx11_noexcept;
  virtual const char *what() const cxx11_noexcept;
};

#define FPTU_DECLARE_EXCEPTION(NAME, BASE)                                     \
  class FPTU_API_TYPE NAME : public BASE {                                     \
    using base = BASE;                                                         \
                                                                               \
  public:                                                                      \
    NAME() cxx11_noexcept;                                                     \
    NAME(const char *details) cxx11_noexcept;                                  \
    NAME(const std::string &details) cxx11_noexcept;                           \
    virtual ~NAME() cxx11_noexcept;                                            \
  }

FPTU_DECLARE_EXCEPTION(invalid_argument, std::invalid_argument);
FPTU_DECLARE_EXCEPTION(invalid_allot, std::invalid_argument);
FPTU_DECLARE_EXCEPTION(invalid_schema, std::invalid_argument);
FPTU_DECLARE_EXCEPTION(tuple_hollow, std::invalid_argument);
FPTU_DECLARE_EXCEPTION(field_absent, std::runtime_error);
FPTU_DECLARE_EXCEPTION(logic_error, std::logic_error);
FPTU_DECLARE_EXCEPTION(type_mismatch, std::logic_error);
FPTU_DECLARE_EXCEPTION(schema_mismatch, std::logic_error);
FPTU_DECLARE_EXCEPTION(index_corrupted, std::logic_error);
FPTU_DECLARE_EXCEPTION(collection_unallowed, std::logic_error);
FPTU_DECLARE_EXCEPTION(collection_required, std::logic_error);
FPTU_DECLARE_EXCEPTION(value_is_denil, std::range_error);
FPTU_DECLARE_EXCEPTION(value_too_long, std::length_error);
FPTU_DECLARE_EXCEPTION(tuple_too_large, std::length_error);
FPTU_DECLARE_EXCEPTION(value_out_of_range, std::out_of_range);
FPTU_DECLARE_EXCEPTION(managed_buffer_required, std::logic_error);
FPTU_DECLARE_EXCEPTION(tuple_overflow, std::logic_error);
FPTU_DECLARE_EXCEPTION(schema_definition_error, std::logic_error);
FPTU_DECLARE_EXCEPTION(schema_no_such_field, std::logic_error);
#undef FPTU_DECLARE_EXCEPTION

} // namespace fptu

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __GNUC__
#pragma GCC visibility pop
#endif
