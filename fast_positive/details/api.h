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

#define FPTU_VERSION_MAJOR 0
#define FPTU_VERSION_MINOR 0

#include "fast_positive/config.h"
#include "fast_positive/details/erthink/erthink_defs.h"

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
