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

#include "fast_positive/details/api.h"
#include "fast_positive/details/bug.h"

#include "fast_positive/details/1Hippeus/actor.h"
#include "fast_positive/details/1Hippeus/buffer.h"
#include "fast_positive/details/1Hippeus/hipagut.h"
#include "fast_positive/details/erthink/erthink_defs.h"
#include "fast_positive/details/erthink/erthink_intrin.h"
#include "fast_positive/details/exceptions.h"
#include "fast_positive/details/memcheck_valgrind.h"

bool hippeus_buffer_enforce_deep_checking;

namespace hippeus {

namespace {
template <bool expect_invalid>
__always_inline bool validate_buffer(const buffer *buf, bool deep_checking) {
  deep_checking |= hippeus_buffer_enforce_deep_checking;

  // if (unlikely(!hippeus_is_readable(buf)))
  //  return false;

  bool valid = false;
  if (expect_invalid) {
    VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(buf,
                                                   sizeof(hippeus_buffer_C));
  }

  if (unlikely(0 > (ptrdiff_t)buf->space))
    goto bailout;

  if (unlikely(buf->ref_counter <= 0))
    goto bailout;

  if (unlikely(!buf->host))
    goto bailout;

  if (unlikely(!HIPAGUT_PROBE(buf->guard_head, head)))
    goto bailout;

  if (unlikely(!HIPAGUT_PROBE(buf->guard_under, under)))
    goto bailout;

  if (buf->is_solid() &&
      unlikely(!HIPAGUT_PROBE_ASIDE(buf->_inplace, over, buf->size())))
    goto bailout;

  if (unlikely(deep_checking))
    probe_pages(buf->begin(), buf->size(), !buf->is_readonly());

  valid = true;

bailout:
  if (expect_invalid) {
    VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(buf,
                                                  sizeof(hippeus_buffer_C));
  }

  return valid;
}

} // namespace

//------------------------------------------------------------------------------

__hot __flatten bool buffer::check(bool deep_checking) const {
  return validate_buffer<false>(this, deep_checking);
}

__cold bool buffer::check_expect_invalid(bool deep_checking) const {
  return validate_buffer<true>(this, deep_checking);
}

__hot __flatten bool buffer::ensure(bool deep_checking) const {
  deep_checking |= hippeus_buffer_enforce_deep_checking;
  FPTU_ENSURE((ptrdiff_t)space > 0);
  FPTU_ENSURE(host.opacity_.uint != 0);
  FPTU_ENSURE(ref_counter > 0);

  HIPAGUT_ENSURE(guard_head, head);
  HIPAGUT_ENSURE(guard_under, under);
  if (is_solid())
    HIPAGUT_ENSURE_ASIDE(_inplace, over, size());

  if (unlikely(deep_checking))
    probe_pages(begin(), size(), !is_readonly());
  return true;
}

static inline void drown(buffer *buf) noexcept {
  buf->ref_counter = -1;
  HIPAGUT_DROWN(buf->guard_head);
  HIPAGUT_DROWN(buf->guard_under);
  if (likely(buf->is_solid()))
    HIPAGUT_DROWN_ASIDE(buf->_inplace, buf->size());
}

__hot __flatten void buffer::init(buffer *self, buffer_tag host, void *payload,
                                  ptrdiff_t payload_bytes) {
  assert(self != nullptr);
  FPTU_ENSURE(payload_bytes >= 0 &&
              (payload != nullptr) == (payload_bytes != 0));

  self->space = uint32_t(payload_bytes);
  self->host.opacity_.uint = host.opacity_.uint;
  self->ref_counter = 1;
  self->_data_offset = payload ? static_cast<uint8_t *>(payload) -
                                     reinterpret_cast<uint8_t *>(self)
                               : 0;

  HIPAGUT_SETUP(self->guard_under, under);
  HIPAGUT_SETUP(self->guard_head, head);
  if (payload == self->_inplace) {
    HIPAGUT_SETUP_ASIDE(self->_inplace, over, payload_bytes);
    assert(self->is_solid());
  } else
    assert(!self->is_solid());

  if (unlikely(hippeus_buffer_enforce_deep_checking)) {
    assert(self->ensure(true));
    if (!self->is_readonly() && self->size()) {
      ::memset(self->begin(), 0xCC, self->size());
      assert(self->check());
    }
  }
}

__hot __flatten const buffer *
buffer::add_reference(const buffer *self) noexcept {
  if (likely(self)) {
    int before_increment =
#ifdef _MSC_VER
        _InterlockedIncrement((volatile long *)&self->ref_counter) - 1;
    static_assert(sizeof(self->ref_counter) == sizeof(long), "WTF?");
#else
        __sync_fetch_and_add(&self->ref_counter, 1);
#endif
    assert(before_increment > 0);
    (void)before_increment;
  }
  return self;
}

__hot __flatten void buffer::detach(const buffer *self) noexcept {
  if (unlikely(self == nullptr))
    return;

  assert(self->check());
  if (unlikely(self->ref_counter != 1)) {
    int after_decrement =
#ifdef _MSC_VER
        _InterlockedDecrement((volatile long *)&self->ref_counter);
    static_assert(sizeof(self->ref_counter) == sizeof(long), "WTF?");
#else
        __sync_sub_and_fetch(&self->ref_counter, 1);
#endif
    if (likely(after_decrement > 0))
      return;
  }

  drown(const_cast<buffer *>(self));
  hippeus_allot_C *allot = nullptr;
  if (self->is_localweak() /* TODO: подумать как сделать надежнее,
    один бит не должен все разваливать. */) {
    if (self->host.raw() <
        uintptr_t(hippeus::buffer_tag::HIPPEUS_TAG_LOCALPTR_THRESHOLD)) {
      /* тег не является адресом аллокатора, вызываем виртуальный деструктор */
      auto local = static_cast<buffer_weak *>(const_cast<buffer *>(self));
      assert(self == local);
      ::delete local;
      return;
    }
    allot = self->host.local_allot();
  } else {
#ifdef HIPPEUS
    allot = binder::tag2allot(self->host);
#endif
  }

  if (unlikely(!allot))
    fptu::throw_invalid_allot();
  allot->repay(allot, const_cast<buffer *>(self), thread_actor());
}

__hot buffer *buffer::borrow(buffer_tag host, std::size_t wanna,
                             bool leastwise) {
  if (unlikely(!host))
    host = hippeus::default_allot_tag();

  hippeus_allot_C *allot = nullptr;
  if (host.flags() & HIPPEUS_LOCALWEAK) {
    allot = host.local_allot();
  } else {
#ifdef HIPPEUS
    allot = binder::tag2allot(host);
#endif
  }
  if (unlikely(!allot))
    fptu::throw_invalid_allot();

  buffer *buf = static_cast<buffer *>(
      allot->borrow(allot, wanna, leastwise, thread_actor()));
  if (unlikely(buf == nullptr))
    FPTU_RAISE_BUG(__LINE__, "allot.borrow() returns NIL", __func__, __FILE__);

  if (leastwise)
    FPTU_ENSURE(buf->size() >= wanna);

  return buf;
}

buffer_weak::~buffer_weak() {}

} /* namespace hippeus */
