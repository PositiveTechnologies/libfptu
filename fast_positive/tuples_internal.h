﻿/*
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

/*
 * libfptu = { Fast Positive Tuples, aka Позитивные Кортежи }
 * Машинно-эффективный формат линейного представления небольших структур данных
 * для (де)сериализации, обмена сообщениями и размещения в разделяемой памяти.
 *
 * Всё будет хорошо. The Future will (be) Positive
 * https://github.com/PositiveTechnologies/libfptu
 *
 * Machine-handy format for linear representation of small data structures
 * for (de)serialization, messaging and placement in shared memory.
 */

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE 1
#endif

#ifndef _ISOC11_SOURCE
#define _ISOC11_SOURCE 1
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#ifndef _THREAD_SAFE
#define _THREAD_SAFE 1
#endif

#ifndef _REENTRANT
#define _REENTRANT 1
#endif

#include "fast_positive/tuples.h"

//------------------------------------------------------------------------------

#if defined(_MSC_VER) && _MSC_FULL_VER < 191526730
#pragma message(                                                               \
    "At least \"Microsoft C/C++ Compiler\" version 19.15.26730 (Visual Studio 2017 15.8) is required.")
#endif /* _MSC_VER */

#if !defined(__cplusplus) || __cplusplus < 201103L
#if defined(_MSC_VER)
#pragma message("Does the \"/Zc:__cplusplus\" option is used?")
#endif
#error "Please use C++11/14/17 compiler to build libfptu"
#endif /* C++11 */

//------------------------------------------------------------------------------

/* Useful macros definition, includes __GNUC_PREREQ/__GLIBC_PREREQ */
#include "fast_positive/details/erthink/erthink_defs.h"

#if (defined(__clang__) && !__CLANG_PREREQ(5, 0)) ||                           \
    (!defined(__clang__) && defined(__GNUC__) && !__GNUC_PREREQ(5, 4))
/* Actualy libfptu was not tested with old compilers.
 * But you could remove this #error and try to continue at your own risk.
 * In such case please don't rise up an issues related ONLY to old compilers. */
#error "libfptu required at least GCC 5.4 compatible C/C++ compiler."
#endif /* __GNUC__ */

#if defined(__GLIBC__) && !__GLIBC_PREREQ(2, 12)
/* Actualy libfptu requires just C99 (e.g glibc >= 2.1), but was
 * not tested with glibc older than 2.12 (from RHEL6). So you could
 * remove this #error and try to continue at your own risk.
 * In such case please don't rise up an issues related ONLY to old glibc. */
#error "libfptu required at least glibc version 2.12 or later."
#endif /* __GLIBC__ */

//------------------------------------------------------------------------------

#include <limits.h>
#include <malloc.h>
#include <string.h>

#include <cinttypes> // for PRId64, PRIu64
#include <cmath>     // for exp2()
#include <cstdarg>   // for va_list
#include <cstdio>    // for _vscprintf()
#include <cstdlib>   // for snprintf()
#include <ctime>     // for gmtime()

//------------------------------------------------------------------------------
#include "fast_positive/details/crutches.h"

#include "fast_positive/details/1Hippeus/buffer.h"
#include "fast_positive/details/1Hippeus/hipagut.h"
#include "fast_positive/details/1Hippeus/hipagut_allocator.h"
#include "fast_positive/details/bug.h"
#include "fast_positive/details/cpu_features.h"
#include "fast_positive/details/erthink/erthink_short_alloc.h"
#include "fast_positive/details/meta.h"
#include "fast_positive/details/nan.h"
#include "fast_positive/details/scan.h"
#include "fast_positive/details/type2genus.h"
#include "fast_positive/details/uio.h"
#include "fast_positive/details/utils.h"
#include "fast_positive/tuples_legacy.h"

namespace fptu {

using onstack_allocation_arena =
    erthink::allocation_arena<true, onstask_allocation_threshold>;

template <typename TYPE>
using onstack_short_allocator =
    erthink::short_alloc<TYPE, onstack_allocation_arena>;

} // namespace fptu
