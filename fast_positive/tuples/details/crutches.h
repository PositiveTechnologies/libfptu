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

#ifdef _MSC_VER

#ifndef snprintf
#define snprintf(buffer, buffer_size, format, ...)                             \
  _snprintf_s(buffer, buffer_size, _TRUNCATE, format, __VA_ARGS__)
#endif /* snprintf */

#ifndef vsnprintf
#define vsnprintf(buffer, buffer_size, format, args)                           \
  _vsnprintf_s(buffer, buffer_size, _TRUNCATE, format, args)
#endif /* vsnprintf */

#ifdef _ASSERTE
#undef assert
#define assert _ASSERTE
#endif

#if _MSC_VER >= 1900 && !defined(PRIuSIZE)
/* LY: MSVC 2015/2017 has buggy/inconsistent PRIuPTR/PRIxPTR macros
 * for internal format-args checker. */
#undef PRIuPTR
#undef PRIiPTR
#undef PRIdPTR
#undef PRIxPTR
#define PRIuPTR "Iu"
#define PRIiPTR "Ii"
#define PRIdPTR "Id"
#define PRIxPTR "Ix"
#define PRIuSIZE "zu"
#define PRIiSIZE "zi"
#define PRIdSIZE "zd"
#define PRIxSIZE "zx"
#endif /* fix PRI*PTR for _MSC_VER */

#endif /* _MSC_VER */

#ifndef PRIuSIZE
#define PRIuSIZE PRIuPTR
#define PRIiSIZE PRIiPTR
#define PRIdSIZE PRIdPTR
#define PRIxSIZE PRIxPTR
#endif /* PRI*SIZE macros */

/*----------------------------------------------------------------------------*/
/* LY: temporary workaround for Elbrus's memcmp() bug. */
#if defined(__e2k__) && !__GLIBC_PREREQ(2, 24)
__extern_C int mdbx_e2k_memcmp_bug_workaround(const void *s1, const void *s2,
                                              size_t n);
__extern_C int mdbx_e2k_strcmp_bug_workaround(const char *s1, const char *s2);
__extern_C int mdbx_e2k_strncmp_bug_workaround(const char *s1, const char *s2,
                                               size_t n);
__extern_C size_t mdbx_e2k_strlen_bug_workaround(const char *s);
__extern_C size_t mdbx_e2k_strnlen_bug_workaround(const char *s, size_t maxlen);

#ifdef __cplusplus
namespace std {
inline int mdbx_e2k_memcmp_bug_workaround(const void *s1, const void *s2,
                                          size_t n) {
  return ::mdbx_e2k_memcmp_bug_workaround(s1, s2, n);
}
inline int mdbx_e2k_strcmp_bug_workaround(const char *s1, const char *s2) {
  return ::mdbx_e2k_strcmp_bug_workaround(s1, s2);
}
inline int mdbx_e2k_strncmp_bug_workaround(const char *s1, const char *s2,
                                           size_t n) {
  return ::mdbx_e2k_strncmp_bug_workaround(s1, s2, n);
}
inline size_t mdbx_e2k_strlen_bug_workaround(const char *s) {
  return ::mdbx_e2k_strlen_bug_workaround(s);
}
inline size_t mdbx_e2k_strnlen_bug_workaround(const char *s, size_t maxlen) {
  return ::mdbx_e2k_strnlen_bug_workaround(s, maxlen);
}
} // namespace std
#endif /* __cplusplus */

#include <string.h>
#include <strings.h>
#undef memcmp
#define memcmp mdbx_e2k_memcmp_bug_workaround
#undef bcmp
#define bcmp mdbx_e2k_memcmp_bug_workaround
#undef strcmp
#define strcmp mdbx_e2k_strcmp_bug_workaround
#undef strncmp
#define strncmp mdbx_e2k_strncmp_bug_workaround
#undef strlen
#define strlen mdbx_e2k_strlen_bug_workaround
#undef strnlen
#define strnlen mdbx_e2k_strnlen_bug_workaround
#endif /* Elbrus's memcmp() bug. */
