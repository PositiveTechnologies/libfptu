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
#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
struct iovec {
  void *iov_base;      /* Starting address */
  std::size_t iov_len; /* Number of bytes to transfer */
};
#else
#include <sys/uio.h> // for struct iovec
#endif

#ifdef __cplusplus

namespace fptu {
struct iovec : public ::iovec {
  iovec() {
    iov_base = nullptr;
    iov_len = 0;
  }

  iovec(const void *data, std::size_t size) {
    iov_base = const_cast<void *>(data);
    iov_len = size;
  }

  iovec(const ::iovec &io) {
    iov_base = io.iov_base;
    iov_len = io.iov_len;
  }

  iovec &operator=(const ::iovec &io) {
    iov_base = io.iov_base;
    iov_len = io.iov_len;
    return *this;
  }

  void set(const void *data, std::size_t size) {
    iov_base = const_cast<void *>(data);
    iov_len = size;
  }

  std::size_t size() const { return iov_len; }
  const uint8_t *data() const { return static_cast<const uint8_t *>(iov_base); }
  const uint8_t *end() const { return data() + size(); }
  uint8_t *data() { return static_cast<uint8_t *>(iov_base); }
  uint8_t *end() { return data() + size(); }

  bool equal(const ::iovec &io) const {
    return this->iov_base == io.iov_base && this->iov_len == io.iov_len;
  }

  bool not_equal(const ::iovec &io) const { return !equal(io); }
};
} // namespace fptu

#endif /* __cplusplus */
