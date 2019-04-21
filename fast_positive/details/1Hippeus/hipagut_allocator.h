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
 *
 * ***************************************************************************
 *
 * Imported (with simplification) from 1Hippeus project.
 * Copyright (c) 2006-2013 Leonid Yuriev <leo@yuriev.ru>.
 */

#pragma once
#include "fast_positive/details/1Hippeus/hipagut.h"
#include "fast_positive/details/api.h"

#ifdef __cplusplus

#include <memory>

namespace hippeus {
/* LY: STL-аллокатор с контролем записи за границы выделяемых участков памяти.
 * Запрашивает у базового аллокатора больше чем требуется на величину защитных
 * зазоров. Записывает ключевые hipagut-метки и проверяет их при освобождении.
 *
 * По-сути аналогичный функционал есть и в glibc (см. MALLOC_CHECK_). Однако
 * иногда удобно иметь независимый уровень явного контроля для подмножества
 * классов.
 */
template <class ALLOCATOR> class hipagut_allocator : public ALLOCATOR {
public:
  using inherited = ALLOCATOR;
  using value_type = typename inherited::value_type;
  using size_type = typename inherited::size_type;
  using difference_type = typename inherited::difference_type;
#if __cplusplus >= 201402L
  using propagate_on_container_move_assignment =
      typename inherited::propagate_on_container_move_assignment;
#endif
#if __cplusplus >= 201703L
  using is_always_equal = typename inherited::is_always_equal;
#endif
#if __cplusplus <= 201703L
  using pointer = typename inherited::pointer;
  using const_pointer = typename inherited::const_pointer;
  using reference = typename inherited::reference;
  using const_reference = typename inherited::const_reference;

  template <typename T> struct rebind {
    using other_unchecked = typename inherited::template rebind<T>::other;
    using other = hipagut_allocator<other_unchecked>;
  };
#endif

  constexpr hipagut_allocator() noexcept {}
  hipagut_allocator(inherited &__a) noexcept : inherited(__a) {}
  constexpr hipagut_allocator(const inherited &__a) noexcept : inherited(__a) {}
  template <typename T>
  hipagut_allocator(hipagut_allocator<T> &__a) noexcept : inherited(__a) {}
  template <typename T>
  constexpr hipagut_allocator(const hipagut_allocator<T> &__a) noexcept
      : inherited(__a) {}

  size_type max_size() const noexcept {
    return
#if __cplusplus > 201703L
        return std::numeric_limits<size_type>::max() / sizeof(value_type)
#else
        inherited::max_size()
#endif
               - hipagut_gap() * 2;
  }

  static constexpr std::size_t hipagut_gap() {
    return (HIPAGUT_SPACE + sizeof(value_type) - 1) / sizeof(value_type);
  }

#if __cplusplus < 201703L
  pointer allocate(size_type n, const void *hint = nullptr) {
    if (unlikely(!n || n > max_size()))
      return nullptr;

    pointer ptr = inherited::allocate(n + hipagut_gap() * 2, hint);
    if (likely(ptr != nullptr)) {
      ptr += hipagut_gap();
      HIPAGUT_SETUP_ASIDE(ptr, chkU, -HIPAGUT_SPACE);
      HIPAGUT_SETUP_ASIDE(ptr, chkO, n * sizeof(value_type));
    }

    return ptr;
  }
#endif

#if __cplusplus == 201703L
  value_type *allocate(size_type n, const void *hint) {
    if (unlikely(!n || n > max_size()))
      return nullptr;

    value_type *ptr = inherited::allocate(n + hipagut_gap() * 2, hint);
    if (likely(ptr != nullptr)) {
      ptr += hipagut_gap();
      HIPAGUT_SETUP_ASIDE(ptr, chkU, -HIPAGUT_SPACE);
      HIPAGUT_SETUP_ASIDE(ptr, chkO, n * sizeof(value_type));
    }

    return ptr;
  }
#endif

#if __cplusplus >= 201703L
  value_type *allocate(size_type n) {
    if (unlikely(!n || n > max_size()))
      return nullptr;

    value_type *ptr = inherited::allocate(n + hipagut_gap() * 2);
    if (likely(ptr != nullptr)) {
      ptr += hipagut_gap();
      HIPAGUT_SETUP_ASIDE(ptr, chkU, -HIPAGUT_SPACE);
      HIPAGUT_SETUP_ASIDE(ptr, chkO, n * sizeof(value_type));
    }

    return ptr;
  }
#endif

  void deallocate(pointer p, size_type n) {
    if (likely(n > 0)) {
      assert(p != nullptr && n <= max_size());

      HIPAGUT_ENSURE_ASIDE(p, chkU, -HIPAGUT_SPACE);
      HIPAGUT_ENSURE_ASIDE(p, chkO, n * sizeof(value_type));

      HIPAGUT_DROWN_ASIDE(p, -HIPAGUT_SPACE);
      HIPAGUT_DROWN_ASIDE(p, n * sizeof(value_type));

      inherited::deallocate(p - hipagut_gap(), n + hipagut_gap() * 2);
    } else
      assert(p == nullptr);
  }
};

template <typename T>
class hipagut_std_allocator : public hipagut_allocator<std::allocator<T>> {
public:
  using inherited = hipagut_allocator<std::allocator<T>>;

  hipagut_std_allocator() throw() {}
  hipagut_std_allocator(const inherited &__a) throw() : inherited(__a) {}
  hipagut_std_allocator(inherited &__a) throw() : inherited(__a) {}

  template <typename TT>
  hipagut_std_allocator(const hipagut_std_allocator<TT> &__a) throw()
      : inherited(__a) {}
};

} /* namespace hippeus */

#endif /* __cplusplus */
