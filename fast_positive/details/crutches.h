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

/*
 * libfptu = { Fast Positive Tuples, aka Позитивные Кортежи }
 *
 * The kind of lightweight linearized tuples, which are extremely handy
 * to machining, including cases with shared memory.
 * Please see README.md at https://github.com/leo-yuriev/libfptu
 *
 * The Future will Positive. Всё будет хорошо.
 *
 * "Позитивные Кортежи" дают легковесное линейное представление небольших
 * JSON-подобных структур в экстремально удобной для машины форме,
 * в том числе при размещении в разделяемой памяти.
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
extern "C" int mdbx_e2k_memcmp_bug_workaround(const void *s1, const void *s2,
                                              std::size_t n);
extern "C" int mdbx_e2k_strcmp_bug_workaround(const char *s1, const char *s2);
extern "C" int mdbx_e2k_strncmp_bug_workaround(const char *s1, const char *s2,
                                               std::size_t n);
extern "C" std::size_t mdbx_e2k_strlen_bug_workaround(const char *s);
extern "C" std::size_t mdbx_e2k_strnlen_bug_workaround(const char *s,
                                                       std::size_t maxlen);
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
