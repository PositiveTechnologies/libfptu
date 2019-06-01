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
#include "fast_positive/details/erthink/erthink_dynamic_constexpr.h"

#include <stddef.h>
#include <stdint.h>

namespace fptu {

class narrow_tagged_pointer_base {
public:
  static constexpr unsigned bits = 4;

private:
  union casting {
    uintptr_t uint;
    void *ptr;
    const void *const_ptr;

#if !defined(__COVERITY__)
    constexpr casting(const casting &) noexcept = default;
    constexpr casting &operator=(const casting &) noexcept = default;
#else
    constexpr casting(const casting &ones) noexcept : uint(ones.uint) {}
    constexpr casting &operator=(const casting &ones) noexcept {
      uint = ones.uint;
      return *this;
    }
#endif
    constexpr casting(void *ptr) noexcept : ptr(ptr) {}
    constexpr casting(const void *ptr) noexcept : const_ptr(ptr) {}
    constexpr casting(uintptr_t uint) noexcept : uint(uint) {}
  };
  casting body_;

  static constexpr uintptr_t tag_mask = (1u << bits) - 1u;
  static constexpr uintptr_t ptr_mask = uintptr_t(~tag_mask);
  static constexpr uintptr_t ptr_xor = 0
      /* This is valid for x86_64 arch due amd64-specific detail about the
       effective user-mode virtual address space. Briefly: the upper 16 bits of
       pointers are always zero in user-mode.
       http://en.wikipedia.org/wiki/X86-64#Canonical_form_addresses

       TODO: Support for other architectures. */
      ;

protected:
  static constexpr uintptr_t p2u(void *ptr) noexcept {
    return casting(ptr).uint;
  }

  static constexpr void *u2p(uintptr_t u) noexcept { return casting(u).ptr; }

public:
  constexpr unsigned tag() const noexcept { return body_.uint & tag_mask; }

  constexpr void *ptr() const noexcept {
    return u2p((body_.uint & ptr_mask) ^ ptr_xor);
  }

  constexpr narrow_tagged_pointer_base() noexcept : body_(ptr_xor) {
    constexpr_assert(ptr() == nullptr && tag() == 0);
  }

  constexpr narrow_tagged_pointer_base(void *ptr, unsigned tag = 0) noexcept
      : body_((p2u(ptr) ^ ptr_xor) | (uintptr_t(tag) << bits)) {
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->ptr() == ptr && this->tag() == tag);
  }

  constexpr narrow_tagged_pointer_base(
      const narrow_tagged_pointer_base &v) noexcept = default;
  cxx14_constexpr narrow_tagged_pointer_base &
  operator=(const narrow_tagged_pointer_base &v) noexcept = default;

  cxx14_constexpr void swap(narrow_tagged_pointer_base &v) noexcept {
    const auto temp = body_;
    body_ = v.body_;
    v.body_ = temp;
  }

  cxx14_constexpr void set_ptr(void *ptr) noexcept {
    body_.uint = (body_.uint & tag_mask) | (p2u(ptr) ^ ptr_xor);
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(this->ptr() == ptr);
  }

  cxx14_constexpr void set_tag(unsigned tag) noexcept {
    body_.uint = (body_.uint & ptr_mask) | (uintptr_t(tag) << bits);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->tag() == tag);
  }

  cxx14_constexpr void set(void *ptr, unsigned tag) noexcept {
    body_.uint = (p2u(ptr) ^ ptr_xor) | (uintptr_t(tag) << bits);
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->ptr() == ptr && this->tag() == tag);
  }
};

//------------------------------------------------------------------------------

class wide_tagged_pointer_base {
public:
  static constexpr unsigned bits = 16;

private:
  union casting {
    uint64_t u64;
    uintptr_t up;
    void *ptr;
    const void *const_ptr;

#if !defined(__COVERITY__)
    constexpr casting(const casting &) noexcept = default;
    constexpr casting &operator=(const casting &) noexcept = default;
#else
    constexpr casting(const casting &ones) noexcept : u64(ones.u64) {}
    constexpr casting &operator=(const casting &ones) noexcept {
      u64 = ones.u64;
      return *this;
    }
#endif
    constexpr casting(void *ptr) noexcept : ptr(ptr) {}
    constexpr casting(const void *ptr) noexcept : const_ptr(ptr) {}
    constexpr casting(uint64_t u64) noexcept : u64(u64) {}
  };
  casting body_;

  static constexpr uint64_t tag_mask = ~(~UINT64_C(0) >> bits);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4310 /* cast truncates of constant value */)
#endif
  static constexpr uintptr_t ptr_mask = uintptr_t(~tag_mask);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  static constexpr uintptr_t ptr_xor = 0
      /* This is valid for x86_64 arch due amd64-specific detail about the
       effective user-mode virtual address space. Briefly: the upper 16 bits of
       pointers are always zero in user-mode.
       http://en.wikipedia.org/wiki/X86-64#Canonical_form_addresses

       TODO: Support for other architectures. */
      ;

  static constexpr uintptr_t p2u(void *ptr) noexcept { return casting(ptr).up; }

  static constexpr void *u2p(uintptr_t u) noexcept { return casting(u).ptr; }

public:
  constexpr unsigned tag() const noexcept { return body_.u64 >> (64 - bits); }

  constexpr void *ptr() const noexcept {
    return u2p((body_.up & ptr_mask) ^ ptr_xor);
  }

  constexpr wide_tagged_pointer_base() noexcept : body_(uint64_t(ptr_xor)) {
    constexpr_assert(ptr() == nullptr && tag() == 0);
  }

  constexpr wide_tagged_pointer_base(void *ptr, unsigned tag = 0) noexcept
      : body_((p2u(ptr) ^ ptr_xor) | (uint64_t(tag) << bits)) {
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->ptr() == ptr && this->tag() == tag);
  }

  constexpr wide_tagged_pointer_base(
      const wide_tagged_pointer_base &v) noexcept = default;
  cxx14_constexpr wide_tagged_pointer_base &
  operator=(const wide_tagged_pointer_base &v) noexcept = default;

  cxx14_constexpr void swap(wide_tagged_pointer_base &v) noexcept {
    const auto temp = body_;
    body_ = v.body_;
    v.body_ = temp;
  }

  cxx14_constexpr void set_ptr(void *ptr) noexcept {
    body_.u64 = (body_.u64 & tag_mask) | (p2u(ptr) ^ ptr_xor);
    constexpr_assert(((p2u(ptr) ^ ptr_xor) & tag_mask) == 0);
    constexpr_assert(this->ptr() == ptr);
  }

  cxx14_constexpr void set_tag(unsigned tag) noexcept {
    body_.u64 = (body_.u64 & ptr_mask) | (uint64_t(tag) << bits);
    constexpr_assert(tag < (1u << bits));
    constexpr_assert(this->tag() == tag);
  }

  cxx14_constexpr void set(void *ptr, unsigned tag) noexcept {
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
  constexpr tagged_pointer(T *ptr, unsigned tag = 0) noexcept
      : base(ptr, tag) {}
  constexpr tagged_pointer() noexcept : base() {}

  void set(T *ptr, unsigned tag) noexcept { base::set(ptr, tag); }
  void set_ptr(void *ptr) noexcept { base::set_ptr(ptr); }
  void set_tag(unsigned tag) noexcept { base::set_tag(tag); }
  constexpr unsigned tag() const noexcept { return base::tag(); }
  constexpr T *get() const noexcept {
    return erthink::constexpr_pointer_cast<T *>(base::ptr());
  }
  constexpr T *operator->() const noexcept { return get(); }
  constexpr reference operator*() const noexcept { return *get(); }
  constexpr reference operator[](std::ptrdiff_t i) const noexcept {
    return get()[i];
  }
};

} // namespace fptu
