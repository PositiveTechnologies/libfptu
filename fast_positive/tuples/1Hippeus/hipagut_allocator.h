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
 *
 * ***************************************************************************
 *
 * Imported (with simplification) from 1Hippeus project.
 * Copyright (c) 2006-2013 Leonid Yuriev <leo@yuriev.ru>.
 */

#pragma once
#include "fast_positive/tuples/1Hippeus/hipagut.h"
#include "fast_positive/tuples/api.h"

#ifdef __cplusplus

#include <limits>
#include <memory>

namespace hippeus {
template <class ALLOCATOR> class hipagut_allocator;
}

// Partial specialization std::allocator_traits<> for hipagut_allocator<>
namespace std {
template <typename ALLOCATOR>
struct allocator_traits<hippeus::hipagut_allocator<ALLOCATOR>>
    : public allocator_traits<ALLOCATOR> {
  using unchecked_allocator_type = ALLOCATOR;
  using unchecked_base = allocator_traits<ALLOCATOR>;
  using allocator_type = hippeus::hipagut_allocator<ALLOCATOR>;

  using value_type = typename unchecked_base::value_type;
  using size_type = typename unchecked_base::size_type;
  /* using pointer = typename unchecked_base::pointer;
  using difference_type = typename unchecked_base::difference_type;
  using const_pointer = typename unchecked_base::const_pointer;
  using reference = typename unchecked_base::reference;
  using const_reference = typename unchecked_base::const_reference; */

  static constexpr size_type hipagut_gap() {
    return (HIPAGUT_SPACE + sizeof(value_type) - 1) / sizeof(value_type);
  }

  static size_type max_size(const allocator_type &a) noexcept {
    return unchecked_base::max_size(a) - hipagut_gap() * 2;
  }

  template <typename T>
  using rebind_alloc = typename allocator_type::template rebind<T>::other;
  template <typename T> using rebind_traits = allocator_traits<rebind_alloc<T>>;
};

} // namespace std

namespace hippeus {
/* LY: STL-аллокатор с контролем записи за границы выделяемых участков памяти.
 * Запрашивает у базового аллокатора больше чем требуется на величину защитных
 * зазоров. Записывает ключевые hipagut-метки и проверяет их при освобождении.
 *
 * По-сути аналогичный функционал есть и в glibc (см. MALLOC_CHECK_). Однако
 * иногда удобно иметь независимый уровень явного контроля для подмножества
 * классов. */
template <class ALLOCATOR> class hipagut_allocator : public ALLOCATOR {
public:
  using unchecked_base = ALLOCATOR;
  using unchecked_traits = std::allocator_traits<unchecked_base>;
  using traits = typename std::allocator_traits<hipagut_allocator>;

  using value_type = typename unchecked_traits::value_type;
  using size_type = typename unchecked_traits::size_type;
  using pointer = typename unchecked_traits::pointer;

  template <typename T /*, typename ...Args*/> struct rebind {
    typedef typename unchecked_base::template rebind<T /*, Args...*/>::other
        other_unchecked;
    typedef hipagut_allocator<other_unchecked> other;
  };

  constexpr hipagut_allocator() noexcept {}
  hipagut_allocator(unchecked_base &a) noexcept : unchecked_base(a) {}
  constexpr hipagut_allocator(const unchecked_base &a) noexcept
      : unchecked_base(a) {}
  template <typename T>
  hipagut_allocator(hipagut_allocator<T> &a) noexcept : unchecked_base(a) {}
  template <typename T>
  constexpr hipagut_allocator(const hipagut_allocator<T> &a) noexcept
      : unchecked_base(a) {}

#if __cplusplus < 201703L
  pointer allocate(size_type n, const void *hint = nullptr) {
    if (unlikely(!n || n > traits::max_size(*this)))
      return nullptr;

    pointer ptr = unchecked_base::allocate(n + traits::hipagut_gap() * 2, hint);
    if (likely(ptr != nullptr)) {
      ptr += traits::hipagut_gap();
      HIPAGUT_SETUP_ASIDE(ptr, chkU, -HIPAGUT_SPACE);
      HIPAGUT_SETUP_ASIDE(ptr, chkO, n * sizeof(value_type));
    }

    return ptr;
  }
#endif

#if __cplusplus == 201703L
  value_type *allocate(size_type n, const void *hint) {
    if (unlikely(!n || n > traits::max_size(*this)))
      return nullptr;

    value_type *ptr =
        unchecked_base::allocate(n + traits::hipagut_gap() * 2, hint);
    if (likely(ptr != nullptr)) {
      ptr += traits::hipagut_gap();
      HIPAGUT_SETUP_ASIDE(ptr, chkU, -HIPAGUT_SPACE);
      HIPAGUT_SETUP_ASIDE(ptr, chkO, n * sizeof(value_type));
    }

    return ptr;
  }
#endif

#if __cplusplus >= 201703L
  value_type *allocate(size_type n) {
    if (unlikely(!n || n > traits::max_size(*this)))
      return nullptr;

    value_type *ptr = unchecked_base::allocate(n + traits::hipagut_gap() * 2);
    if (likely(ptr != nullptr)) {
      ptr += traits::hipagut_gap();
      HIPAGUT_SETUP_ASIDE(ptr, chkU, -HIPAGUT_SPACE);
      HIPAGUT_SETUP_ASIDE(ptr, chkO, n * sizeof(value_type));
    }

    return ptr;
  }
#endif

  void deallocate(pointer p, size_type n) {
    if (likely(n > 0)) {
      assert(p != nullptr && n <= traits::max_size(*this));

      HIPAGUT_ENSURE_ASIDE(p, chkU, -HIPAGUT_SPACE);
      HIPAGUT_ENSURE_ASIDE(p, chkO, n * sizeof(value_type));

      HIPAGUT_DROWN_ASIDE(p, -HIPAGUT_SPACE);
      HIPAGUT_DROWN_ASIDE(p, n * sizeof(value_type));

      unchecked_base::deallocate(p - traits::hipagut_gap(),
                                 n + traits::hipagut_gap() * 2);
    } else
      assert(p == nullptr);
  }
};

template <class T, class U>
bool operator==(hipagut_allocator<T> const &x,
                hipagut_allocator<U> const &y) noexcept {
  return static_cast<typename hipagut_allocator<T>::unchecked_base>(x) ==
         static_cast<typename hipagut_allocator<U>::unchecked_base>(y);
}

template <class T, class U>
bool operator!=(hipagut_allocator<T> const &x,
                hipagut_allocator<U> const &y) noexcept {
  return !(x == y);
}

} // namespace hippeus

namespace hippeus {
template <typename T>
class hipagut_std_allocator : public hipagut_allocator<std::allocator<T>> {
public:
  using base = hipagut_allocator<std::allocator<T>>;
  using traits = typename base::traits;

  hipagut_std_allocator() throw() {}
  hipagut_std_allocator(const base &a) throw() : base(a) {}
  hipagut_std_allocator(base &a) throw() : base(a) {}

  template <typename TT>
  hipagut_std_allocator(const hipagut_std_allocator<TT> &a) throw() : base(a) {}
};

} /* namespace hippeus */

#endif /* __cplusplus */
