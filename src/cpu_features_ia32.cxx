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

#include "fast_positive/erthink/erthink_defs.h"
#include "fast_positive/erthink/erthink_intrin.h"
#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/details/cpu_features.h"
#include <string.h> // for memset()

namespace fptu {

const ia32_cpu_features cpu_features;

/* Crutch for Intel Compiler (copy&paste from GCC's cpuid.h file */
#if defined(__INTEL_COMPILER) && defined(__GNUC__) && !defined(__cpuid)
#define __cpuid(level, a, b, c, d)                                             \
  __asm__("cpuid\n\t" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(level))

#define __cpuid_count(level, count, a, b, c, d)                                \
  __asm__("cpuid\n\t"                                                          \
          : "=a"(a), "=b"(b), "=c"(c), "=d"(d)                                 \
          : "0"(level), "2"(count))

static __inline unsigned __get_cpuid_max(unsigned __ext, unsigned *__sig) {
  unsigned __eax, __ebx, __ecx, __edx;

#ifndef __x86_64__
  /* See if we can use cpuid.  On AMD64 we always can.  */
  __asm__("pushf{l|d}\n\t"
          "pushf{l|d}\n\t"
          "pop{l}\t%0\n\t"
          "mov{l}\t{%0, %1|%1, %0}\n\t"
          "xor{l}\t{%2, %0|%0, %2}\n\t"
          "push{l}\t%0\n\t"
          "popf{l|d}\n\t"
          "pushf{l|d}\n\t"
          "pop{l}\t%0\n\t"
          "popf{l|d}\n\t"
          : "=&r"(__eax), "=&r"(__ebx)
          : "i"(0x00200000));

  if (!((__eax ^ __ebx) & 0x00200000))
    return 0;
#endif /* __x86_64__ */

  /* Host supports cpuid.  Return highest supported cpuid input value.  */
  __cpuid(__ext, __eax, __ebx, __ecx, __edx);

  if (__sig)
    *__sig = __ebx;

  return __eax;
}
#endif /* Crutch for Intel Compiler (copy&paste from GCC's cpuid.h file */

void ia32_cpu_features::clear() { memset(this, 0, sizeof(ia32_cpu_features)); }

bool ia32_cpu_features::fetch() {
  clear();
  bool rc = false;

#if defined(__GNUC__) || defined(__clang__)

  uint32_t unused_eax, unused_ebx, cpuid_max;

  cpuid_max = __get_cpuid_max(0, NULL);
  if (cpuid_max >= 1) {
    __cpuid_count(1, 0, unused_eax, basic.ebx, basic.ecx, basic.edx);
    if (cpuid_max >= 7)
      __cpuid_count(7, 0, unused_eax, extended_7.ebx, extended_7.ecx,
                    extended_7.edx);
    rc = true;
  }
  cpuid_max = __get_cpuid_max(0x80000000, NULL);
  if (cpuid_max >= 0x80000001) {
    __cpuid_count(0x80000001, 0, unused_eax, unused_ebx, extended_80000001.ecx,
                  extended_80000001.edx);
    if (cpuid_max >= 0x80000007)
      __cpuid_count(0x80000007, 0, unused_eax, unused_ebx,
                    extended_80000007.ecx, extended_80000007.edx);
    rc = true;
  }

#elif defined(_MSC_VER) && !defined(__clang__)

  int info[4];
  __cpuid(info, 0);
  unsigned cpuid_max = info[0];
  if (cpuid_max >= 1) {
    __cpuidex(info, 1, 0);
    basic.ebx = info[1];
    basic.ecx = info[2];
    basic.edx = info[3];
    if (cpuid_max >= 7) {
      __cpuidex(info, 7, 0);
      extended_7.ebx = info[1];
      extended_7.ecx = info[2];
      extended_7.edx = info[3];
    }
    rc = true;
  }

  __cpuid(info, 0x80000000);
  cpuid_max = info[0];
  if (cpuid_max >= 0x80000001) {
    __cpuidex(info, 0x80000001, 0);
    extended_80000001.ecx = info[2];
    extended_80000001.edx = info[3];
    if (cpuid_max >= 0x80000007) {
      __cpuidex(info, 0x80000007, 0);
      extended_80000007.ecx = info[2];
      extended_80000007.edx = info[3];
    }
    rc = true;
  }

#else
#warning "FIXME: Unsupported compiler"
#endif

  return rc;
}

} // namespace fptu
