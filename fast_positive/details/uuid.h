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

#pragma once
#include "fast_positive/details/api.h"

#include "fast_positive/details/fixed_binary.h"
#include <stdint.h>

typedef struct FPTU_API_TYPE fptu_uuid {
  fptu_binary128_t bin128;
#ifdef __cplusplus
#endif
} fptu_uuid_t;

#ifdef __cplusplus
namespace fptu {
typedef ::fptu_uuid_t uuid_t;
} // namespace fptu
#endif /* __cplusplus */
