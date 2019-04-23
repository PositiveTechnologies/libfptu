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

#pragma once
#include "fast_positive/details/api.h"

#ifdef __cplusplus
#include <cmath>
#include <cstdint>
#include <ctime>
#else
#include <math.h>
#include <stdint.h>
#include <time.h>
#endif /* __cplusplus */

#include "windows_mustdie.h"

#include "fast_positive/details/warnings_push_pt.h"

/* Представление времени.
 *
 * В формате фиксированной точки 32-dot-32:
 *   - В старшей "целой" части секунды по UTC, буквально как выдает time(),
 *     но без знака. Это отодвигает проблему 2038-го года на 2106,
 *     требуя взамен аккуратности при вычитании.
 *   - В младшей "дробной" части неполные секунды в 1/2**32 долях.
 *
 * Эта форма унифицирована с "Positive Hiper100re" и одновременно достаточно
 * удобна в использовании. Поэтому настоятельно рекомендуется использовать
 * именно её, особенно для хранения и передачи данных. */

union fptu_datetime_C {
  uint64_t fixedpoint;
  struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t fractional;
    uint32_t utc;
#else
    uint32_t utc;
    uint32_t fractional;
#endif /* byte order */
  };
};

/* Возвращает текущее время в правильной форме.
 *
 * Аргумент grain_ns задает желаемую точность в наносекундах, в зависимости от
 * которой будет использован CLOCK_REALTIME, либо CLOCK_REALTIME_COARSE.
 *
 * Положительные значения grain_ns, включая нуль, трактуются как наносекунды.
 *
 * Отрицательные же означают количество младших бит, которые НЕ требуются в
 * результате и будут обнулены. Таким образом, отрицательные значения grain_ns
 * позволяют запросить текущее время, одновременно с "резервированием" младших
 * бит результата под специфические нужды.
 *
 * В конечном счете это позволяет существенно экономить на системных вызовах
 * и/или обращении к аппаратуре. В том числе не выполнять системный вызов,
 * а ограничиться использованием механизма vdso (прямое чтение из открытой
 * страницы данных ядра). В зависимости от запрошенной точности,
 * доступной аппаратуры и актуальном режиме работы ядра, экономия может
 * составить до сотен и даже тысяч раз.
 *
 * Следует понимать, что реальная точность зависит от актуальной конфигурации
 * аппаратуры и ядра ОС. Проще говоря, запрос текущего времени с grain_ns = 1
 * достаточно абсурден и вовсе не гарантирует такой точности результата. */
__extern_C FPTU_API union fptu_datetime_C fptu_now(int grain_ns);
__extern_C FPTU_API union fptu_datetime_C fptu_now_fine(void);
__extern_C FPTU_API union fptu_datetime_C fptu_now_coarse(void);

//------------------------------------------------------------------------------

#ifdef __cplusplus
namespace fptu {

class FPTU_API datetime_t {
  fptu_datetime_C value_;

  union casting {
    fptu_datetime_C C_union;
    uint64_t u64;
    constexpr casting(uint64_t u64) : u64(u64) {}
  };

  static fptu_datetime_C constexpr cast(uint64_t u) {
    return casting(u).C_union;
  }

  static constexpr uint32_t ns_per_second = 1000000000u;
  static constexpr uint32_t us_per_second = 1000000u;
  static constexpr uint32_t ms_per_second = 1000u;
  static constexpr uint32_t ns100_per_second = ns_per_second / 100;
  static constexpr uint64_t Gregorian_UTC_offset_100ns =
      UINT64_C(/* UTC offset from 1601-01-01 */ 116444736000000000);

  static constexpr uint_fast32_t units2fractional(uint_fast32_t value,
                                                  uint_fast32_t factor) {
    constexpr_assert(value < factor);
    return (uint64_t(value) << 32) / factor;
  }

  static constexpr uint64_t scale_in(uint64_t value, uint_fast32_t factor) {
    return ((value / factor) << 32) | units2fractional(value % factor, factor);
  }

  static constexpr uint_fast32_t fractional2units(uint_fast32_t value,
                                                  uint_fast32_t factor) {
    constexpr_assert(value <= UINT32_MAX);
    return (uint64_t(value) * factor) >> 32;
  }

#ifdef _FILETIME_
  static uint64_t filetime_to_utc_100ns(const FILETIME &FileTime) {
    return (uint64_t(FileTime.dwHighDateTime) << 32 | FileTime.dwLowDateTime) -
           Gregorian_UTC_offset_100ns;
  }
#endif

  explicit constexpr datetime_t(uint64_t u) : value_(casting(u).C_union) {}

public:
  static constexpr uint_fast32_t ns2fractional(uint_fast32_t ns) {
    return units2fractional(ns, ns_per_second);
  }
  static constexpr uint_fast32_t fractional2ns(uint_fast32_t fraction) {
    return fractional2units(fraction, ns_per_second);
  }
  static constexpr uint_fast32_t us2fractional(uint_fast32_t us) {
    return units2fractional(us, us_per_second);
  }
  static constexpr uint_fast32_t fractional2us(uint_fast32_t fraction) {
    return fractional2units(fraction, us_per_second);
  }
  static constexpr uint_fast32_t ms2fractional(uint_fast32_t ms) {
    return units2fractional(ms, ms_per_second);
  }
  static constexpr uint_fast32_t fractional2ms(uint_fast32_t fraction) {
    return fractional2units(fraction, ms_per_second);
  }

  double fractional2seconds() const {
    return std::ldexp(value_.fractional, -32);
  }
  double seconds() const { return fractional2seconds() + value_.utc; }
  uint32_t utc_seconds() const { return value_.utc; }
  uint32_t fractional() const { return value_.fractional; }
  uint64_t fixedpoint_32dot32() const { return value_.fixedpoint; }

  datetime_t() = default;
  constexpr datetime_t(const datetime_t &) = default;
  datetime_t &operator=(const datetime_t &) = default;
  constexpr datetime_t(const union fptu_datetime_C &src) : value_(src) {}
  datetime_t &operator=(const union fptu_datetime_C &src) {
    value_ = src;
    return *this;
  }

  constexpr datetime_t(uint64_t units, uint_fast32_t units_per_second)
      : datetime_t(scale_in(units, units_per_second)) {}

  static datetime_t now_fine() { return datetime_t(fptu_now_fine()); }
  static datetime_t now_coarse() { return datetime_t(fptu_now_coarse()); }
  static datetime_t now(int grain_ns) { return datetime_t(fptu_now(grain_ns)); }

  constexpr datetime_t(const time_t *utc) : datetime_t(uint64_t(*utc) << 32) {}

  static constexpr datetime_t from_fixedpoint_32dot32(uint64_t u32dot32) {
    return datetime_t(u32dot32);
  }

  static constexpr datetime_t from_seconds(unsigned utc) {
    return datetime_t(uint64_t(utc) << 32);
  }

  static constexpr datetime_t from_milliseconds(uint64_t ms) {
    return datetime_t(ms, ms_per_second);
  }

  static constexpr datetime_t from_usec(uint64_t us) {
    return datetime_t(us, us_per_second);
  }

  static constexpr datetime_t from_nsec(uint64_t ns) {
    return datetime_t(ns, ns_per_second);
  }

  static constexpr datetime_t from_100ns(uint64_t ns100) {
    return datetime_t(ns100, ns100_per_second);
  }

#ifdef HAVE_TIMESPEC_TV_NSEC
  constexpr datetime_t(const struct timespec &ts)
      : datetime_t(uint64_t(ts.tv_sec) << 32 |
                   ns2fractional(uint_fast32_t(ts.tv_nsec))) {}

  constexpr static datetime_t from_timespec(const struct timespec &ts) {
    return datetime_t(ts);
  }
#endif /* HAVE_TIMESPEC_TV_NSEC */

#ifdef HAVE_TIMEVAL_TV_USEC
  constexpr datetime_t(const struct timeval &tv)
      : datetime_t(uint64_t(tv.tv_sec) << 32 |
                   us2fractional(uint_fast32_t(tv.tv_usec))) {}

  constexpr static datetime_t from_timeval(const struct timeval &tv) {
    return datetime_t(tv);
  }
#endif /* HAVE_TIMEVAL_TV_USEC */

#ifdef _FILETIME_
  constexpr datetime_t(const FILETIME &FileTime)
      : datetime_t(filetime_to_utc_100ns(FileTime), ns100_per_second) {}

  constexpr static datetime_t from_filetime(const FILETIME &FileTime) {
    return datetime_t(FileTime);
  }
  constexpr static datetime_t from_filetime(const FILETIME *pFileTime) {
    return datetime_t(*pFileTime);
  }
#endif /* _FILETIME_ */

  operator const fptu_datetime_C &() const { return value_; }
  operator fptu_datetime_C &() { return value_; }

  friend inline bool operator==(const datetime_t &lhs, const datetime_t &rhs) {
    return lhs.value_.fixedpoint == rhs.value_.fixedpoint;
  }
  friend inline bool operator!=(const datetime_t &lhs, const datetime_t &rhs) {
    return !operator==(lhs, rhs);
  }
  friend inline bool operator<(const datetime_t &lhs, const datetime_t &rhs) {
    return lhs.value_.fixedpoint < rhs.value_.fixedpoint;
  }
  friend inline bool operator>(const datetime_t &lhs, const datetime_t &rhs) {
    return operator<(rhs, lhs);
  }
  friend inline bool operator<=(const datetime_t &lhs, const datetime_t &rhs) {
    return !operator>(lhs, rhs);
  }
  friend inline bool operator>=(const datetime_t &lhs, const datetime_t &rhs) {
    return !operator<(lhs, rhs);
  }
};
} // namespace fptu

using fptu_datetime_t = fptu::datetime_t;

#else

typdef union fptu_datetime_C fptu_datetime_t;

#endif /* __cplusplus */

#include "fast_positive/details/warnings_pop.h"
