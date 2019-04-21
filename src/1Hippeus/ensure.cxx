/*
    Copyright (c) 2013 Leonid Yuriev <leo@yuriev.ru>.
    Copyright (c) 2013 Peter-Service LLC.
    Copyright (c) 2013 Other contributors as noted in the AUTHORS file.

    This file is part of 1Hippeus.

    1Hippeus is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    1Hippeus is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <hippeus/ensure.h>

void (*hippeus_throw_handler)(const char *file, uintptr_t line,
                              const char *function, hippeus_troubles error,
                              const char *note);

__cold void hippeus_throw_compact(uintptr_t compact__throuble_line,
                                  const char *note) {
  uintptr_t line = compact__throuble_line / hippeus_troubles_max;
  hippeus_troubles error =
      (hippeus_troubles)(compact__throuble_line % hippeus_troubles_max);
  hippeus_throw_diag("binary", line, "text", error, note);
}

__cold void hippeus_throw_diag(const char *file, uintptr_t line,
                               const char *function, hippeus_troubles error,
                               const char *note) {
  if (hippeus_throw_handler)
    hippeus_throw_handler(file, line, function, error, note);

  __hippeus_exception(file, line, function, error, note);
}

#define calle_address                                                          \
  ((uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)))

__cold void hippeus_throw_corrupt(const char *note) {
  hippeus_throw_diag("binary", calle_address, "text", corruption, note);
}

__cold void hippeus_throw_nomem(const char *note) {
  hippeus_throw_diag("binary", calle_address, "text", bad_alloc, note);
}

__cold void hippeus_throw_invarg(const char *note) {
  hippeus_throw_diag("binary", calle_address, "text", invalid_argument, note);
}

__cold void hippeus_throw_orange(const char *note) {
  hippeus_throw_diag("binary", calle_address, "text", out_of_range, note);
}

__cold void hippeus_throw_system(const char *note) {
  hippeus_throw_diag("binary", calle_address, "text", system_error, note);
}

static __cold const char *trouble2str(hippeus_troubles trouble) {
  switch (trouble) {
  case not_implemented:
    return "not_implemented";
  case assertion:
    return "assertion";
  case corruption:
    return "corruption";
  case no_buffer:
    return "no_buffer";

  case bad_alloc:
    return "bad_alloc";
  case bad_array_new_length:
    return "bad_array_new_length";
  case bad_weak_ptr:
    return "bad_weak_ptr";

  case internal_range:
    return "internal_range";
  case internal_overflow:
    return "internal_overflow";
  case internal_underflow:
    return "internal_underflow";
  case system_error:
    return "system_error";

  case domain_error:
    return "domain_error";
  case invalid_argument:
    return "invalid_argument";
  case length_error:
    return "length_error";
  case out_of_range:
    return "out_of_range";
  case hippeus_troubles_max:;
  }
  return "noname/unknown";
}

#ifdef __KERNEL__

extern "C" {
#include <linux/bug.h>
#include <linux/kernel.h>
#include <linux/printk.h>
}

__cold void __hippeus_exception(const char *file, uintptr_t line,
                                const char *function, hippeus_troubles error,
                                const char *note) {
  panic("1Hippeus caught the EXCEPTION/BUG:\n"
        "  %s (#%u) with note '%s'\n"
        "  function '%s'\n"
        "      file '%s', line %u\n\n",
        trouble2str(error), (note && *note) ? note : "none",
        (function && *function) ? function : "?", (file && *file) ? file : "?",
        line);
}
}

#else /* __KERNEL__ */

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <exception>
#include <iostream>
#include <memory>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

/* Prototype should match libc runtime. ISO POSIX (2003) & LSB 3.1 */
__extern_C void __assert_fail(const char *assertion, const char *file,
                              unsigned line,
                              const char *function) __nothrow __noreturn;

unsigned hippeus_ensure_termination_disabled;

static void terminate(const char *what) {
  if (hippeus_ensure_termination_disabled !=
      HIPPEUS_ENSURE_TERMINATION_DISABLED) {
    std::cerr << what;
    std::cout.flush();
    std::cerr.flush();
    ::fflush(nullptr);
    std::terminate();
  }
}

class assert_fail : public std::domain_error {
public:
  explicit assert_fail(const std::string &what_arg)
      : std::domain_error(what_arg) {}

  explicit assert_fail(const char *what_arg) : std::domain_error(what_arg) {}

  ~assert_fail() { terminate(what()); }
};

class hipagut_corruption : public std::runtime_error {
public:
  explicit hipagut_corruption(const std::string &what_arg)
      : std::runtime_error(what_arg) {}

  explicit hipagut_corruption(const char *what_arg)
      : std::runtime_error(what_arg) {}

  ~hipagut_corruption() { terminate(what()); }
};

__cold void __hippeus_exception(const char *file, uintptr_t line,
                                const char *function, hippeus_troubles trouble,
                                const char *note) {

  if (HIPPEUS_ASSERT_CHECK)
    switch (trouble) {
    case assertion:
      __assert_fail(note, file, line, function);
    case corruption:
      __assert_fail("hipagut-guard", file, line, function);
    default:;
    }

  std ::stringstream info;
  info << "1Hippeus caught the EXCEPTION/BUG:\n"
       << std ::endl
       << " " << trouble2str(trouble) << " (#" << (uintptr_t)(trouble) << ")";

  if (note && *note)
    info << " with note '" << note << '\'';
  info << std ::endl;

  if (function && *function)
    info << "  function '" << function << '\'' << std ::endl;

  if (file && *file) {
    info << "      file '" << file << '\'';
    if (line)
      info << ", line " << line;

    info << std ::endl;
  }

  info << std ::endl;
  switch (trouble) {
  case internal_range:
    throw std::range_error(info.str());
  case internal_overflow:
    throw std::overflow_error(info.str());
  case internal_underflow:
    throw std::underflow_error(info.str());
  case system_error:
    throw std::system_error(errno, std::generic_category(), info.str());

  case assertion:
    throw assert_fail(info.str());
  case domain_error:
    throw std::domain_error(info.str());
  case invalid_argument:
    throw std::invalid_argument(info.str());
  case bad_array_new_length:
    // throw std::bad_array_new_length(info.str());
  case length_error:
    throw std::length_error(info.str());
  case out_of_range:
    throw std::out_of_range(info.str());

  case bad_weak_ptr:
    // throw std::bad_weak_ptr(info.str());
    throw std::logic_error(info.str());

  case corruption:
    throw hipagut_corruption(info.str());

  case not_implemented:
  case no_buffer:
  case bad_alloc:
      // throw std::bad_alloc(info.str());
      ;

  case hippeus_troubles_max:;
  }

  throw std::runtime_error(info.str());
}

#endif /* __KERNEL__ */
