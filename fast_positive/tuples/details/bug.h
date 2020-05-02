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
#include "fast_positive/tuples/api.h"

#ifndef BUG_PROVIDE_LINENO
#define BUG_PROVIDE_LINENO 1
#endif

#ifndef BUG_PROVIDE_CONDITION
#define BUG_PROVIDE_CONDITION 1
#endif

#ifndef BUG_PROVIDE_FUNCTION
#define BUG_PROVIDE_FUNCTION 1
#endif

#ifndef BUG_PROVIDE_FILENAME
#define BUG_PROVIDE_FILENAME 1
#endif

namespace fptu {

class bug_location {
#if BUG_PROVIDE_LINENO
  const unsigned line_;
#endif
#if BUG_PROVIDE_CONDITION
  const char *const condition_;
#endif
#if BUG_PROVIDE_FUNCTION
  const char *const function_;
#endif
#if BUG_PROVIDE_FILENAME
  const char *const filename_;
#endif

public:
  cxx11_constexpr bug_location(unsigned line, const char *condition,
                               const char *function, const char *filename)
      :
#if BUG_PROVIDE_LINENO
        line_(line)
#endif
#if BUG_PROVIDE_CONDITION
        ,
        condition_(condition)
#endif
#if BUG_PROVIDE_FUNCTION
        ,
        function_(function)
#endif
#if BUG_PROVIDE_FILENAME
        ,
        filename_(filename)
#endif
  {
#if !BUG_PROVIDE_LINENO
    (void)line;
#endif
#if !BUG_PROVIDE_CONDITION
    (void)condition;
#endif
#if !BUG_PROVIDE_FUNCTION
    (void)function;
#endif
#if !BUG_PROVIDE_FILENAME
    (void)filename;
#endif
  }

  bug_location(const bug_location &&) = delete;

  unsigned line() const {
#if BUG_PROVIDE_LINENO
    return line_;
#else
    return 0;
#endif
  }

  const char *condition() const {
#if BUG_PROVIDE_CONDITION
    return condition_;
#else
    return "";
#endif
  }

  const char *function() const {
#if BUG_PROVIDE_FUNCTION
    return function_;
#else
    return "";
#endif
  }

  const char *filename() const {
#if BUG_PROVIDE_FILENAME
    return filename_;
#else
    return "";
#endif
  }
};

FPTU_API void raise_bug(const bug_location &what_and_where);

#define FPTU_RAISE_BUG(line, condition, function, file)                        \
  do {                                                                         \
    static cxx11_constexpr_var ::fptu::bug_location bug(line, condition,       \
                                                        function, file);       \
    ::fptu::raise_bug(bug);                                                    \
  } while (0)

#define FPTU_ENSURE(condition)                                                 \
  do                                                                           \
    if (unlikely(!(condition)))                                                \
      FPTU_RAISE_BUG(__LINE__, #condition, __func__, __FILE__);                \
  while (0)

#define FPTU_NOT_IMPLEMENTED()                                                 \
  FPTU_RAISE_BUG(__LINE__, "not implemented", __func__, __FILE__);

} // namespace fptu
