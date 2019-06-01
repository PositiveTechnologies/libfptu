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

#include "fast_positive/tuples.h"

#include "fast_positive/details/bug.h"
#include "fast_positive/details/ro.h"
#include "fast_positive/details/rw.h"

#include "fast_positive/details/erthink/erthink_optimize4size.h"
#include "fast_positive/details/exceptions.h"

#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif

namespace fptu __dll_visibility_default {

__cold void raise_bug(const bug_location &what_and_where) {
  throw bug(what_and_where);
}

__cold bug::bug(const bug_location &location) noexcept
    : std::runtime_error(format("fptu-bug: %s.%s at %s:%u",
                                location.condition(), location.function(),
                                location.filename(), location.line())),
      location_(location) {}

__cold bug::~bug() noexcept {}

//------------------------------------------------------------------------------

__cold bad_tuple::bad_tuple(const char *details) : base(details) {}
__cold bad_tuple::bad_tuple(const std::string &details) : base(details) {}
__cold bad_tuple::~bad_tuple() noexcept {}

__cold bad_tuple_ro::bad_tuple_ro(const fptu::schema *schema,
                                  const void *const ptr,
                                  const std::size_t bytes, const char *details)
    : bad_tuple(
          format("fptu: Invalid ro-tuple {%p:%zd}: %s", ptr, bytes, details)),
      schema_(schema), ptr_(ptr), bytes_(bytes) {
  FPTU_ENSURE(details != nullptr && *details != '\0');
}

__cold bad_tuple_ro::bad_tuple_ro(const fptu::schema *schema,
                                  const details::tuple_ro *tuple,
                                  const char *details)
    : bad_tuple_ro(schema, tuple, tuple ? tuple->size() : 0, details) {}

__cold bad_tuple_ro::bad_tuple_ro(const fptu::schema *schema,
                                  const details::tuple_ro *tuple,
                                  const std::string &details)
    : bad_tuple_ro(schema, tuple, details.c_str()) {}
__cold bad_tuple_ro::bad_tuple_ro(const fptu::schema *schema,
                                  const details::tuple_ro *tuple)
    : bad_tuple_ro(schema, tuple,
                   details::tuple_ro::audit(tuple, schema, true)) {}
__cold bad_tuple_ro::~bad_tuple_ro() noexcept {}

__cold bad_tuple_rw::bad_tuple_rw(const details::tuple_rw *tuple,
                                  const char *details)
    : bad_tuple(format("fptu: Invalid rw-tuple %p: %s", tuple, details)),
      tuple_(tuple) {
  FPTU_ENSURE(details != nullptr && *details != '\0');
}
__cold bad_tuple_rw::bad_tuple_rw(const details::tuple_rw *tuple,
                                  const std::string &details)
    : bad_tuple_rw(tuple, details.c_str()) {}
__cold bad_tuple_rw::bad_tuple_rw(const details::tuple_rw *tuple)
    : bad_tuple_rw(tuple, tuple->audit()) {}
__cold bad_tuple_rw::~bad_tuple_rw() noexcept {}

//------------------------------------------------------------------------------

__cold insufficient_space::insufficient_space(size_t index_space,
                                              std::size_t data_space) noexcept
    : std::bad_alloc(), index_space(index_space), data_space(data_space) {}

__cold const char *insufficient_space::what() const noexcept {
  return "insufficient space";
}
__cold insufficient_space::~insufficient_space() noexcept {}

__cold __noreturn void throw_insufficient_space(size_t index,
                                                std::size_t data) {
  throw insufficient_space(index, data);
}

__cold __noreturn void throw_tuple_overflow() { throw tuple_overflow(); }

//------------------------------------------------------------------------------

#define FPTU_DEFINE_EXCEPTION(NAME, MESSAGE)                                   \
  __cold NAME::NAME() noexcept : base(MESSAGE) {}                              \
  __cold NAME::NAME(const char *details) noexcept : base(details) {}           \
  __cold NAME::NAME(const std::string &details) noexcept : base(details) {}    \
  __cold NAME::~NAME() noexcept {}

FPTU_DEFINE_EXCEPTION(invalid_argument, "fptu: invalid argument")
FPTU_DEFINE_EXCEPTION(invalid_allot,
                      "fptu: 1Hipeus: invalid or unsupported allocator tag")
FPTU_DEFINE_EXCEPTION(invalid_schema, "fptu: invalid schema")
FPTU_DEFINE_EXCEPTION(tuple_hollow, "fptu: hollow tuple")
FPTU_DEFINE_EXCEPTION(field_absent, "fptu: no such field in the tuple")
FPTU_DEFINE_EXCEPTION(logic_error, "fptu: logic error")
FPTU_DEFINE_EXCEPTION(type_mismatch, "fptu: field type mismatch")
FPTU_DEFINE_EXCEPTION(schema_mismatch, "fptu: tuple schema mismatch")
FPTU_DEFINE_EXCEPTION(index_corrupted,
                      "fptu: field already removed (or index corrupted)")
FPTU_DEFINE_EXCEPTION(collection_unallowed, "fptu: collection unallowed")
FPTU_DEFINE_EXCEPTION(collection_required, "fptu: collection required")
FPTU_DEFINE_EXCEPTION(value_is_denil, "fptu: value is prohibited NIL")
FPTU_DEFINE_EXCEPTION(value_too_long, "fptu: value is too long")
FPTU_DEFINE_EXCEPTION(tuple_too_large, "fptu: tuple is too large")
FPTU_DEFINE_EXCEPTION(value_out_of_range, "fptu: value out of range")
FPTU_DEFINE_EXCEPTION(managed_buffer_required,
                      "fptu: managed 1Hippeus's buffer required")
FPTU_DEFINE_EXCEPTION(tuple_overflow, "fptu: tuple size limit reached")

FPTU_DEFINE_EXCEPTION(schema_definition_error,
                      "fptu: unspecified field definition error")
FPTU_DEFINE_EXCEPTION(schema_no_such_field, "fptu: no such field in the schema")

#undef FPTU_DEFINE_EXCEPTION

__cold __noreturn void throw_schema_definition_error(const char *details) {
  throw schema_definition_error(details);
}

__cold __noreturn void throw_invalid_argument() { throw invalid_argument(); }
__cold __noreturn void throw_invalid_argument(const char *details) {
  throw invalid_argument(details);
}

__cold __noreturn void throw_invalid_allot() { throw invalid_allot(); }
__cold __noreturn void throw_invalid_allot(const char *details) {
  throw invalid_allot(details);
}

__cold __noreturn void throw_invalid_schema() { throw invalid_schema(); }
__cold __noreturn void throw_invalid_schema(const char *details) {
  throw invalid_schema(details);
}

__cold __noreturn void throw_schema_mismatch() { throw schema_mismatch(); }
__cold __noreturn void throw_schema_mismatch(const char *details) {
  throw schema_mismatch(details);
}

__cold __noreturn void throw_tuple_hollow() { throw tuple_hollow(); }
__cold __noreturn void throw_type_mismatch() { throw type_mismatch(); }
__cold __noreturn void throw_index_corrupted() { throw index_corrupted(); }
__cold __noreturn void throw_collection_unallowed() {
  throw collection_unallowed();
}
__cold __noreturn void throw_collection_required() {
  throw collection_required();
}
__cold __noreturn void throw_field_absent() { throw field_absent(); }
__cold __noreturn void throw_value_denil() { throw value_is_denil(); }
__cold __noreturn void throw_value_too_long() { throw value_too_long(); }
__cold __noreturn void throw_tuple_too_large() { throw tuple_too_large(); }
__cold __noreturn void throw_value_range() { throw value_out_of_range(); }

__cold __noreturn void throw_tuple_bad(const fptu::schema *schema,
                                       const void *const ptr,
                                       const std::size_t bytes,
                                       const char *details) {
  throw bad_tuple_ro(schema, ptr, bytes, details);
}
__cold __noreturn void throw_tuple_bad(const fptu::schema *schema,
                                       const details::tuple_ro *tuple) {
  throw bad_tuple_ro(schema, tuple);
}
__cold __noreturn void throw_tuple_bad(const fptu::schema *schema,
                                       const details::tuple_ro *tuple,
                                       const char *details) {
  throw bad_tuple_ro(schema, tuple, details);
}

__cold __noreturn void throw_tuple_bad(const details::tuple_rw *tuple) {
  throw bad_tuple_rw(tuple);
}

__cold __noreturn void throw_tuple_bad(const details::tuple_rw *tuple,
                                       const char *details) {
  throw bad_tuple_rw(tuple, details);
}

} // namespace fptu__dll_visibility_default

#ifdef __GNUC__
#pragma GCC visibility pop
#endif
