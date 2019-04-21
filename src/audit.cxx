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

/*
 * libfptu = { Fast Positive Tuples, aka Позитивные Кортежи }
 *
 * The kind of lightweight linearized tuples, which are extremely handy
 * to machining, including cases with shared memory.
 * Please see README.md at https://github.com/leo-yuriev/libfptu
 *
 * The Future will Positive. Всё будет хорошо.
 *
 * "Позитивные Кортежи" дают легковесное линейное представление небольших
 * JSON-подобных структур в экстремально удобной для машины форме,
 * в том числе при размещении в разделяемой памяти.
 */

#include "fast_positive/tuples_internal.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace fptu {
namespace details {
namespace {

class auditor {
  using offset = uint16_t;
  using interval = std::pair<offset, offset>;
  using map_vector = std::vector<interval, onstack_short_allocator<interval>>;
  map_vector map_;
  std::size_t holes_count_, holes_volume_;

  struct map_less {
    bool operator()(const interval &a, const interval &b) const noexcept {
      return a.first < b.first;
    }
    bool operator()(const offset &a, const interval &b) const noexcept {
      return a < b.first;
    }
    bool operator()(const interval &a, const offset &b) const noexcept {
      return a.first < b;
    }
    bool operator()(const offset &a, const offset &b) const noexcept {
      return a < b;
    }
  };

  bool check_map() const;
  bool map_insert(const offset begin, const offset end);

public:
  std::size_t holes_count() const { return holes_count_; }
  std::size_t holes_volume() const { return holes_volume_; }
  bool map_empty() const { return map_.empty(); }
  void map_reset(unsigned limit) {
    map_.clear();
    map_.reserve(limit);
  }
  bool map_insert(const ptrdiff_t begin, const ptrdiff_t end) {
    FPTU_ENSURE(begin < end && size_t(begin) <= UINT16_MAX &&
                size_t(end) <= UINT16_MAX);
    return map_insert(uint16_t(begin), uint16_t(end));
  }

  unsigned map_begin() const { return map_.empty() ? 0 : map_.front().first; }
  unsigned map_end() const { return map_.empty() ? 0 : map_.back().second; }

  bool map_have_holes() const {
    assert(check_map());
    return map_.size() > 1;
  }

  std::size_t map_items() const { return map_.size(); }

  auditor(onstack_allocation_arena &area, std::size_t limit)
      : map_(area), holes_count_(0), holes_volume_(0) {
    static_assert(sizeof(interval) == 2 * sizeof(offset), "WTF?");
    FPTU_ENSURE(limit <= fptu::max_fields);
    map_.reserve(limit);
  }

  const char *check_chunk(const unit_t *chunk_begin,
                          const std::size_t chunk_units,
                          const unit_t *begin_payload,
                          const unit_t *end_payload);

  const char *check_loose(const field_loose *loose, const unit_t *begin_payload,
                          const unit_t *end_payload);
};

bool __maybe_unused auditor::check_map() const {
  for (size_t i = 0; i < map_.size(); ++i) {
    if (i > 0) {
      assert(map_[i - 1].first < map_[i].first);
      assert(map_[i - 1].second < map_[i].first);
      if (map_[i - 1].first >= map_[i].first ||
          map_[i - 1].second >= map_[i].first)
        return false;
    }
    assert(map_[i].first < map_[i].second);
    if (map_[i].first >= map_[i].second)
      return false;
  }
  return true;
}

bool auditor::map_insert(const offset begin, const offset end) {
  assert(begin < end);
  assert(check_map());

  auto const next = std::upper_bound(map_.begin(), map_.end(), end, map_less());
  assert(next == map_.end() || end < next->first) /* paranoia */;

  if (next == map_.begin()) {
    map_.insert(next, interval(begin, end));
    assert(check_map());
    return true;
  }

  auto const here = std::prev(next);
  if (here->second < begin) {
    map_.insert(next, interval(begin, end));
    assert(check_map());
    return true;
  }

  assert(begin <= here->second && here->first <= end) /* paranoia */;
  if (here->first == end) {
    /* should merge */
    if (here != map_.begin()) {
      auto const prev = std::prev(here);
      if (prev->second > begin)
        return false /* overlapped */;
      if (prev->second == begin) {
        /* merge more */
        prev->second = here->second;
        map_.erase(here);
        assert(check_map());
        return true;
      }
    }
    here->first = begin;
    assert(check_map());
    return true;
  }

  if (likely(begin == here->second)) {
    here->second = end;
    assert(check_map());
    return true;
  }

  assert(begin < here->second && here->first < end) /* paranoia */;
  return false /* overlapped */;
}

const char *auditor::check_chunk(const unit_t *chunk_begin,
                                 const std::size_t chunk_units,
                                 const unit_t *begin_payload,
                                 const unit_t *end_payload) {
  assert(chunk_units > 0);
  if (unlikely(chunk_begin < begin_payload))
    return "chunk_begin < begin_payload";
  if (unlikely(chunk_units > fptu::max_field_bytes))
    return "chunk_units > fptu::max_field_bytes";
  if (unlikely(chunk_begin + chunk_units > end_payload))
    return "chunk_end > end_payload";

  const ptrdiff_t offset_begin = chunk_begin - begin_payload;
  const ptrdiff_t offset_end = offset_begin + chunk_units;
  assert(offset_begin >= 0 && offset_begin <= UINT16_MAX);
  assert(offset_end > offset_begin && offset_end <= UINT16_MAX + 1);
  if (unlikely(!map_insert(uint16_t(offset_begin), uint16_t(offset_end))))
    return "chunk overlaps with field's payload or hole";
  return nullptr;
}

const char *auditor::check_loose(const field_loose *loose,
                                 const unit_t *begin_payload,
                                 const unit_t *end_payload) {
  const genus type = tag2genus(loose->genius_and_id);
  if (is_inplaced(type))
    return nullptr;

  if (type == genus::hole) {
    const std::size_t units = loose->hole_get_units();
    holes_count_ += 1;
    if (units == 0) {
      if (unlikely(loose->relative.have_payload()))
        return "tuple.pure_hole.payload != null";
      return nullptr;
    }
    if (!loose->relative.have_payload())
      return "tuple.non_pure_hole.payload == null";
    holes_volume_ += units;
    return check_chunk(loose->hole_begin(), units, begin_payload, end_payload);
  }

  if (is_fixed_size(type)) {
    if (!loose->relative.have_payload())
      return "tuple.fixed_size_loose_field.payload == null";
    return check_chunk(loose->relative.payload()->flat,
                       loose_units_dynamic(type), begin_payload, end_payload);
  }

  /* stretchy */
  if (loose->relative.have_payload())
    return check_chunk(loose->relative.payload()->flat,
                       loose->relative.payload()->stretchy.brutto_units(type),
                       begin_payload, end_payload);
  return nullptr;
}
} // namespace

__hot const char *audit_tuple(const fptu::schema *const schema,
                              const field_loose *const index_begin,
                              const void *const pivot, const void *const end,
                              const audit_flags flags,
                              audit_holes_info &holes_info) {
  const field_loose *const index_end = static_cast<const field_loose *>(pivot);
  const unit_t *const payload_begin = static_cast<const unit_t *>(pivot);
  const unit_t *const payload_end = static_cast<const unit_t *>(end);
  assert(index_begin <= index_end && payload_begin <= payload_end);

  const ptrdiff_t payload_bytes =
      static_cast<const char *>(end) - static_cast<const char *>(pivot);
  const std::size_t preplaced_bytes = schema ? schema->preplaced_bytes() : 0;
  assert(payload_bytes >= 0 && ptrdiff_t(preplaced_bytes) >= 0);

  const char *trouble;
  onstack_allocation_arena area;
  auditor validator(area, index_end - index_begin +
                              (schema ? schema->stretchy_preplaced() : 0));
  for (const field_loose *loose = index_end; --loose >= index_begin;) {
    trouble = validator.check_loose(loose, payload_begin, payload_end);
    if (unlikely(trouble))
      return trouble;
  }

  if (unlikely(validator.holes_count() > UINT16_MAX / 2))
    return "tool many holes";
  if (unlikely(validator.holes_volume() > UINT16_MAX))
    return "tool large holes volume";
  holes_info.holes_count = uint16_t(validator.holes_count());
  holes_info.holes_volume = uint16_t(validator.holes_volume());

  const bool tuple_have_preplaced =
      (flags & audit_flags::audit_tuple_have_preplaced) != 0;
  if (tuple_have_preplaced && !schema)
    return "schema is required for tuples with preplaced fields";
  if (tuple_have_preplaced != (preplaced_bytes != 0))
    return "preplaced-fields presence mismatch with schema";

  if (preplaced_bytes) {
    if (!validator.map_empty() &&
        unlikely(preplaced_bytes > validator.map_begin()))
      return "schema.preplaced > tuple.loose_payload";
    if (unlikely(ptrdiff_t(preplaced_bytes) > payload_bytes))
      return "schema.preplaced > tuple.whole_payload";
    for (const auto field_token : schema->tokens()) {
      if (!field_token.is_preplaced())
        continue;
      const std::size_t preplaced_offset = field_token.preplaced_offset();
      assert(preplaced_offset + preplaced_bytes_dynamic(field_token.type()) <=
             preplaced_bytes);
      if (is_fixed_size(field_token.type()))
        continue;

      const field_preplaced *const target =
          erthink::constexpr_pointer_cast<const field_preplaced *>(
              static_cast<const char *>(pivot) + preplaced_offset);

      if (!target->relative.have_payload())
        continue;
      const auto payload = target->relative.payload();
      trouble = validator.check_chunk(
          payload->flat, payload->stretchy.brutto_units(field_token.type()),
          payload_begin, payload_end);
      if (unlikely(trouble))
        return trouble;
    }
  }

  const auto map_begin = validator.map_begin();
  if (unlikely(tuple_have_preplaced != (map_begin > 0)))
    return tuple_have_preplaced
               ? "preplaced-flag is ON but corresponding fields absent"
               : "preplaced-flag is OFF but tuple have hole for ones";

  if (unlikely(map_begin != preplaced_bytes))
    return tuple_have_preplaced
               ? "preplaced-flag is ON but corresponding fields absent"
               : "preplaced-flag is OFF but tuple have hole for ones";

  const unit_t *const allocated_end = payload_begin + validator.map_end();
  if (unlikely(allocated_end != payload_end))
    return (allocated_end > payload_end) ? "allocated beyond end of tuple"
                                         : "lose space at the end of tuple";

  if (unlikely(validator.map_have_holes()))
    return "tuple have unaccounted holes";

  if (flags & audit_flags::audit_adjacent_holes) {
    validator.map_reset(holes_info.holes_count);
    std::size_t count = 0;
    for (const field_loose *loose = index_end; --loose >= index_begin;) {
      if (!loose->is_hole() || loose->hole_get_units() == 0)
        continue;
      const bool inserted =
          validator.map_insert(loose->hole_begin() - payload_begin,
                               loose->hole_end() - payload_begin);
      assert(loose->hole_begin() >= payload_begin &&
             loose->hole_begin() - payload_begin < UINT16_MAX);
      assert(loose->hole_end() >= payload_begin &&
             loose->hole_end() - payload_begin < UINT16_MAX);
      assert(inserted);
      (void)inserted;
      if (unlikely(++count != validator.map_items()))
        return "tuple have unmerged adjacent holes";
    }
  }

  if ((flags & audit_flags::audit_tuple_sorted_loose) != 0 &&
      !std::is_sorted(index_begin, index_end,
                      [](const field_loose &a, const field_loose &b) {
                        return a.genius_and_id < b.genius_and_id;
                      }))
    return "loose fields mis-sorted";

  return nullptr;
}

} // namespace details
} // namespace fptu
