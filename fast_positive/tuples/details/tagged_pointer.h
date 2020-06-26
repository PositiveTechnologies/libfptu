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
#include "fast_positive/erthink/erthink_casting.h"
#include "fast_positive/tuples/api.h"

#include <stddef.h>
#include <stdint.h>

namespace fptu {

class narrow_tagged_pointer_base {
public:
  static cxx11_constexpr_var unsigned bits = 4;

private:
  union casting {
    uintptr_t uint;
    void *ptr;
    const void *const_ptr;

#if !defined(__COVERITY__)
    cxx11_constexpr casting(const casting &) cxx11_noexcept = default;
    cxx11_constexpr casting &
    operator=(const casting &) cxx11_noexcept = default;
#else
    cxx11_constexpr casting(const casting &ones) cxx11_noexcept
        : uint(ones.uint) {}
    cxx11_constexpr casting &operator=(const casting &ones) cxx11_noexcept {
      uint = ones.uint;
      return *this;
    }
#endif
    cxx11_constexpr casting(void *ptr) cxx11_noexcept : ptr(ptr) {}
    cxx11_constexpr casting(const void *ptr) cxx11_noexcept : const_ptr(ptr) {}
    cxx11_constexpr casting(uintptr_t uint) cxx11_noexcept : uint(uint) {}
  };
  casting body_;

  static cxx11_constexpr_var uintptr_t tag_mask = (1u << bits) - 1u;
  static cxx11_constexpr_var uintptr_t ptr_mask = uintptr_t(~tag_mask);
  static cxx11_constexpr_var uintptr_t ptr_xor = 0
      /* This is valid for x86_64 arch due amd64-specific detail about the
       effective user-mode virtual address space. Briefly: the upper 16 bits of
       pointers are always zero in user-mode.
       http://en.wikipedia.org/wiki/X86-64#Canonical_form_addresses

       TODO: Support for other architectures. */
      ;

protected:
  static cxx11_constexpr uintptr_t p2u(void *ptr) cxx11_noexcept {
    return casting(ptr).uint;
  }

  static cxx11_constexpr void *u2p(uintptr_t u) cxx11_noexcept {
    return casting(u).ptr;
  }

public:
  cxx11_constexpr unsigned tag() const cxx11_noexcept {
    return body_.uint & tag_mask;
  }

  cxx11_constexpr void *ptr() const cxx11_noexcept {
    return u2p((body_.uint & ptr_mask) ^ ptr_xor);
  }

  cxx11_constexpr narrow_tagged_pointer_base() cxx11_noexcept : body_(ptr_xor) {
    constexpr_assert(ptr() == nullptr && tag() == 0);
  }

  cxx11_constexpr narrow_tagged_pointer_base(void *ptr,
                                             unsigned tag = 0) cxx11_noexcept
      : body_((p2u(ptr) ^ ptr_xor) | (uintptr_t(tag) << bits)) {
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->ptr() == ptr && this->tag() == tag);
  }

  cxx11_constexpr narrow_tagged_pointer_base(
      const narrow_tagged_pointer_base &v) cxx11_noexcept = default;
  cxx14_constexpr narrow_tagged_pointer_base &
  operator=(const narrow_tagged_pointer_base &v) cxx11_noexcept = default;

  cxx14_constexpr void swap(narrow_tagged_pointer_base &v) cxx11_noexcept {
    const auto temp = body_;
    body_ = v.body_;
    v.body_ = temp;
  }

  cxx14_constexpr void set_ptr(void *ptr) cxx11_noexcept {
    body_.uint = (body_.uint & tag_mask) | (p2u(ptr) ^ ptr_xor);
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(this->ptr() == ptr);
  }

  cxx14_constexpr void set_tag(unsigned tag) cxx11_noexcept {
    body_.uint = (body_.uint & ptr_mask) | (uintptr_t(tag) << bits);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->tag() == tag);
  }

  cxx14_constexpr void set(void *ptr, unsigned tag) cxx11_noexcept {
    body_.uint = (p2u(ptr) ^ ptr_xor) | (uintptr_t(tag) << bits);
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->ptr() == ptr && this->tag() == tag);
  }
};

//------------------------------------------------------------------------------

class wide_tagged_pointer_base {
public:
  static cxx11_constexpr_var unsigned bits = 16;

private:
  union casting {
    uint64_t u64;
    uintptr_t up;
    void *ptr;
    const void *const_ptr;

#if !defined(__COVERITY__)
    cxx11_constexpr casting(const casting &) cxx11_noexcept = default;
    cxx11_constexpr casting &
    operator=(const casting &) cxx11_noexcept = default;
#else
    cxx11_constexpr casting(const casting &ones) cxx11_noexcept
        : u64(ones.u64) {}
    cxx11_constexpr casting &operator=(const casting &ones) cxx11_noexcept {
      u64 = ones.u64;
      return *this;
    }
#endif
    cxx11_constexpr casting(void *ptr) cxx11_noexcept : ptr(ptr) {}
    cxx11_constexpr casting(const void *ptr) cxx11_noexcept : const_ptr(ptr) {}
    cxx11_constexpr casting(uint64_t u64) cxx11_noexcept : u64(u64) {}
  };
  casting body_;

  static cxx11_constexpr_var uint64_t tag_mask = ~(~UINT64_C(0) >> bits);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4310 /* cast truncates of constant value */)
#endif
  static cxx11_constexpr_var uintptr_t ptr_mask = uintptr_t(~tag_mask);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  static cxx11_constexpr_var uintptr_t ptr_xor = 0
      /* This is valid for x86_64 arch due amd64-specific detail about the
       effective user-mode virtual address space. Briefly: the upper 16 bits of
       pointers are always zero in user-mode.
       http://en.wikipedia.org/wiki/X86-64#Canonical_form_addresses

       TODO: Support for other architectures. */
      ;

  static cxx11_constexpr uintptr_t p2u(void *ptr) cxx11_noexcept {
    return casting(ptr).up;
  }

  static cxx11_constexpr void *u2p(uintptr_t u) cxx11_noexcept {
    return casting(u).ptr;
  }

public:
  cxx11_constexpr unsigned tag() const cxx11_noexcept {
    return body_.u64 >> (64 - bits);
  }

  cxx11_constexpr void *ptr() const cxx11_noexcept {
    return u2p((body_.up & ptr_mask) ^ ptr_xor);
  }

  cxx11_constexpr wide_tagged_pointer_base() cxx11_noexcept
      : body_(uint64_t(ptr_xor)) {
    constexpr_assert(ptr() == nullptr && tag() == 0);
  }

  cxx11_constexpr wide_tagged_pointer_base(void *ptr,
                                           unsigned tag = 0) cxx11_noexcept
      : body_((p2u(ptr) ^ ptr_xor) | (uint64_t(tag) << bits)) {
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->ptr() == ptr && this->tag() == tag);
  }

  cxx11_constexpr wide_tagged_pointer_base(const wide_tagged_pointer_base &v)
      cxx11_noexcept = default;
  cxx14_constexpr wide_tagged_pointer_base &
  operator=(const wide_tagged_pointer_base &v) cxx11_noexcept = default;

  cxx14_constexpr void swap(wide_tagged_pointer_base &v) cxx11_noexcept {
    const auto temp = body_;
    body_ = v.body_;
    v.body_ = temp;
  }

  cxx14_constexpr void set_ptr(void *ptr) cxx11_noexcept {
    body_.u64 = (body_.u64 & tag_mask) | (p2u(ptr) ^ ptr_xor);
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(this->ptr() == ptr);
  }

  cxx14_constexpr void set_tag(unsigned tag) cxx11_noexcept {
    body_.u64 = (body_.u64 & ptr_mask) | (uint64_t(tag) << bits);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->tag() == tag);
  }

  cxx14_constexpr void set(void *ptr, unsigned tag) cxx11_noexcept {
    body_.u64 = (p2u(ptr) ^ ptr_xor) | (uint64_t(tag) << bits);
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->ptr() == ptr && this->tag() == tag);
  }
};

//------------------------------------------------------------------------------

template <class T, class BASE> class tagged_pointer : public BASE {
public:
  using reference = typename std::add_lvalue_reference<T>::type;
  using base = BASE;
  cxx11_constexpr tagged_pointer(T *ptr, unsigned tag = 0) cxx11_noexcept
      : base(ptr, tag) {}
  cxx11_constexpr tagged_pointer() cxx11_noexcept : base() {}

  void set(T *ptr, unsigned tag) cxx11_noexcept { base::set(ptr, tag); }
  void set_ptr(void *ptr) cxx11_noexcept { base::set_ptr(ptr); }
  void set_tag(unsigned tag) cxx11_noexcept { base::set_tag(tag); }
  cxx11_constexpr unsigned tag() const cxx11_noexcept { return base::tag(); }
  cxx11_constexpr T *get() const cxx11_noexcept {
    return erthink::constexpr_pointer_cast<T *>(base::ptr());
  }
  cxx11_constexpr T *operator->() const cxx11_noexcept { return get(); }
  cxx11_constexpr reference operator*() const cxx11_noexcept { return *get(); }
  cxx11_constexpr reference operator[](std::ptrdiff_t i) const cxx11_noexcept {
    return get()[i];
  }
};

} // namespace fptu
