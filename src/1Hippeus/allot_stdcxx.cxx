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
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/exceptions.h"
#include "fast_positive/details/utils.h"

#include "fast_positive/details/1Hippeus/actor.h"
#include "fast_positive/details/1Hippeus/buffer.h"
#include "fast_positive/details/1Hippeus/hipagut.h"
#include "fast_positive/details/1Hippeus/hipagut_allocator.h"

namespace {
class buffer_allocator : public hippeus_allot_t,
                         public hippeus::hipagut_std_allocator<char> {};
} // namespace

static hippeus_buffer_t *borrow(hippeus_allot_t *allot, std::size_t wanna_size,
                                bool leastwise, hippeus_actor_t actor) {
  assert(allot == &hippeus_allot_stdcxx);
  (void)leastwise;
  (void)actor;
  static_assert(sizeof(buffer_allocator) == sizeof(hippeus_allot_t), "WTF?");

  if (unlikely(wanna_size < 1 || wanna_size > fptu::buffer_limit))
    fptu::throw_invalid_argument(
        "requested allocation of a buffer of invalid size");

  const size_t overhead =
      buffer_allocator::traits::hipagut_gap() * 2 + sizeof(void *) * 2;
  const size_t bytes =
      fptu::utils::ceil(
          wanna_size + hippeus::buffer_solid::space_overhead() + overhead, 64) -
      overhead;

  void *ptr =
      erthink::constexpr_pointer_cast<buffer_allocator *>(allot)->allocate(
          bytes);
  auto buffer =
      new (ptr) hippeus::buffer_solid(hippeus::buffer_tag(allot, false), bytes);
  assert(buffer->space + hippeus::buffer_solid::space_overhead() == bytes);
  return buffer;
}

static void repay(hippeus_allot_t *allot, hippeus_buffer_t *buffer,
                  hippeus_actor_t actor) {
  assert(allot == &hippeus_allot_stdcxx);
  (void)actor;
  static_assert(sizeof(buffer_allocator) == sizeof(hippeus_allot_t), "WTF?");
  static_cast<buffer_allocator *>(allot)->deallocate(
      erthink::constexpr_pointer_cast<char *>(buffer),
      buffer->space + hippeus::buffer_solid::space_overhead());
}

static bool validate(const hippeus_allot_t *allot, bool probe_only,
                     bool deep_checking) {
  assert(allot == &hippeus_allot_stdcxx);
  (void)allot;
  (void)probe_only;
  (void)deep_checking;
  return true;
}

hippeus_allot_t hippeus_allot_stdcxx = {4096 - 16, 0, borrow, repay, validate};

namespace hippeus { /* FIXME: TODO */
}
