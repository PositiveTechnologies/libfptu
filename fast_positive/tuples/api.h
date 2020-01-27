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

#define FPTU_VERSION_MAJOR 0
#define FPTU_VERSION_MINOR 2

#include "fast_positive/erthink/erthink_defs.h"
#include "fast_positive/tuples/config.h"

#if defined(fptu_EXPORTS)
#define FPTU_API __dll_export
#elif defined(fptu_IMPORTS)
#define FPTU_API __dll_import
#else
#define FPTU_API
#endif /* fptu_EXPORTS */

#ifdef __cplusplus
#if defined(__clang__) || __has_attribute(type_visibility)
#define FPTU_API_TYPE FPTU_API __attribute__((type_visibility("default")))
#else
#define FPTU_API_TYPE FPTU_API
#endif
#else
#define FPTU_API_TYPE
#endif
