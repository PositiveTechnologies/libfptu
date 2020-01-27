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
struct FPTU_API_TYPE iovec : public ::iovec {
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
