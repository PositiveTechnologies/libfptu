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

#pragma once

#include "fast_positive/details/api.h"
#include "fast_positive/details/erthink/erthink_arch.h"

namespace fptu {

#if defined(__ia32__)
struct FPTU_API_TYPE ia32_cpu_features {
  struct {
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
  } basic /* https://en.wikipedia.org/wiki/CPUID#EAX=1:_Processor_Info_and_Feature_Bits
           */
      ,
      extended_7 /* https://en.wikipedia.org/wiki/CPUID#EAX=7,_ECX=0:_Extended_Features
                  */
      ;

  struct {
    uint32_t ecx;
    uint32_t edx;
  } extended_80000001 /* https://en.wikipedia.org/wiki/CPUID#EAX=80000001h:_Extended_Processor_Info_and_Feature_Bits
                       */
      ;

  struct {
    uint32_t ecx;
    uint32_t edx;
  } extended_80000007 /*  Advanced Power Management Information */;

  //----------------------------------------------------------------------------

  void clear();
  bool fetch();
  ia32_cpu_features() { fetch(); }

  unsigned clflush_size() const { return (uint8_t)(basic.ebx >> 8); }

  unsigned apic_id() const { return (uint8_t)(basic.ebx >> 24); }

  /* Time Stamp Counter */
  bool has_TSC() const { return (basic.edx & (1 << 4)) != 0; }

  bool has_CMPXCHG8() const { return (basic.edx & (1 << 8)) != 0; }

  bool has_CLFLUSH() const { return (basic.edx & (1 << 19)) != 0; }

  bool has_SSE() const { return (basic.edx & (1 << 25)) != 0; }

  bool has_SSE2() const { return (basic.edx & (1 << 26)) != 0; }

  bool has_SSE3() const { return (basic.ecx & (1 << 0)) != 0; }

  /* Carry-less Multiplication (CLMUL) */
  bool has_PCLMULQDQ() const { return (basic.ecx & (1 << 1)) != 0; }

  /* Supplemental SSE3 */
  bool has_SSSE3() const { return (basic.ecx & (1 << 9)) != 0; }

  /* Fused multiply-add (FMA3) */
  bool has_FMA() const { return (basic.ecx & (1 << 12)) != 0; }

  bool has_CMPXCHG16B() const { return (basic.ecx & (1 << 13)) != 0; }

  bool has_SSE41() const { return (basic.ecx & (1 << 19)) != 0; }

  bool has_SSE42() const { return (basic.ecx & (1 << 20)) != 0; }

  bool has_MOVBE() const { return (basic.ecx & (1 << 22)) != 0; }

  bool has_POPCNT() const { return (basic.ecx & (1 << 23)) != 0; }

  /* Vector AES */
  bool has_AES() const { return (basic.ecx & (1 << 25)) != 0; }

  bool has_AVX() const { return (basic.ecx & (1 << 28)) != 0; }

  bool has_RDRAND() const { return (basic.ecx & (1 << 31)) != 0; }

  /* Bit Manipulation Instruction Set 1 */
  bool has_BMI1() const { return (extended_7.ebx & (1 << 3)) != 0; }

  /* Hardware Lock Elision */
  bool has_HLE() const { return (extended_7.ebx & (1 << 4)) != 0; }

  bool has_AVX2() const { return (extended_7.ebx & (1 << 5)) != 0; }

  /* Bit Manipulation Instruction Set 2 */
  bool has_BMI2() const { return (extended_7.ebx & (1 << 8)) != 0; }

  /* Restricted Transactional Memory */
  bool has_RTM() const { return (extended_7.ebx & (1 << 11)) != 0; }

  bool has_RDSEED() const { return (extended_7.ebx & (1 << 18)) != 0; }

  /* Multi-Precision Add-Carry Instruction Extensions */
  bool has_ADX() const { return (extended_7.ebx & (1 << 19)) != 0; }

  bool has_CLFLUSHOPT() const { return (extended_7.ebx & (1 << 23)) != 0; }

  bool has_CLWB() const { return (extended_7.ebx & (1 << 24)) != 0; }

  bool has_SHA() const { return (extended_7.ebx & (1 << 29)) != 0; }

  /* Vector AES-NI */
  bool has_VAES() const { return (extended_7.ecx & (1 << 9)) != 0; }

  /* Vector Carry-less Multiplication (CLMUL) */
  bool has_VPCLMULQDQ() const { return (extended_7.ecx & (1 << 10)) != 0; }

  bool has_RDTSCP() const { return (extended_80000001.edx & (1 << 27)) != 0; }

  /* Advanced bit manipulation (lzcnt and popcnt) */
  bool has_ABM() const { return (extended_80000001.ecx & (1 << 5)) != 0; }

  /* Combined mask-shift & Scalar streaming store instructions */
  bool has_SSE4a() const { return (extended_80000001.ecx & (1 << 6)) != 0; }

  /* Misaligned SSE mode */
  bool has_MisalignSSE() const {
    return (extended_80000001.ecx & (1 << 7)) != 0;
  }

  /* 4 operands fused multiply-add */
  bool has_FMA4() const { return (extended_80000001.ecx & (1 << 16)) != 0; }

  /* Trailing Bit Manipulation */
  bool has_TMB() const { return (extended_80000001.ecx & (1 << 21)) != 0; }
};

FPTU_API extern const ia32_cpu_features cpu_features;
#endif /* __ia32__ */

} // namespace fptu
