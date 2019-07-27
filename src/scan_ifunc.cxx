/*
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

#include "fast_positive/details/cpu_features.h"
#include "fast_positive/details/erthink/erthink_defs.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/scan.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fptu {
namespace details {

#ifndef fptu_scan

ERTHINK_DEFINE_IFUNC(FPTU_API, const field_loose *, fptu_scan,
                     (const field_loose *begin, const field_loose *end,
                      uint16_t genius_and_id),
                     (begin, end, genius_and_id), fptu_scan_resolver)

ERTHINK_IFUNC_RESOLVER_API(FPTU_API)
__cold scan_func_t fptu_scan_resolver() {
#ifdef __ia32__
  if (cpu_features.has_AVX2())
    return fptu_scan_AVX2;
  else if (cpu_features.has_AVX())
    return fptu_scan_AVX;
  else if (cpu_features.has_SSE2())
    return fptu_scan_SSE2;
#else
#warning "FIXME: Support for other architectures"
#endif
  return fptu_scan_unroll;
}

#endif /* fptu_scan */

} // namespace details
} // namespace fptu
