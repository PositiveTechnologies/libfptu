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

#include "fast_positive/details/cpu_features.h"
#include "fast_positive/details/erthink/erthink_arch.h"
#include "fast_positive/details/erthink/erthink_short_alloc.h"
#include "fast_positive/details/meta.h"
#include "fast_positive/details/rw.h"
#include "fast_positive/details/scan.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "fast_positive/details/erthink/erthink_optimize4speed.h"
#include "fast_positive/details/warnings_push_pt.h"

//------------------------------------------------------------------------------

namespace fptu {
namespace details {

namespace {

class gear : public tuple_rw {
  friend class tuple_rw;

  std::pair<field_loose *, field_loose *>
  lookup_adjacent_holes(const unit_t *chunk_begin,
                        const unit_t *chunk_end) noexcept;
  std::pair<field_loose *, field_loose *>
  lookup_adjacent_holes(const field_loose *hole) noexcept {
    assert(hole->hole_get_units() > 0);
    return lookup_adjacent_holes(hole->hole_begin(), hole->hole_end());
  }

  void mark_unsorted() noexcept { /* TODO */
  }

  void mark_sorted() noexcept { /* TODO */
  }

  inline void trim_hole(const field_loose *hole) noexcept;
  inline unit_t *tail_alloc(unsigned units);
  inline field_loose *index_alloc(size_t notify_data_space);
  inline field_loose *merge_holes(field_loose *first,
                                  field_loose *second) noexcept;

  enum class hole_search_mode { exactly_size, any_suitable, best_fit };
  template <hole_search_mode MODE>
  inline __pure_function field_loose *lookup_hole(unsigned units) noexcept;

  inline relative_payload *alloc_data(unsigned units, field_loose *const hole);
  relative_payload *alloc_data(unsigned units) {
    assert(units > 0 && units < bytes2units(max_tuple_bytes_netto));
    return alloc_data(units, lookup_hole<hole_search_mode::best_fit>(units));
  }

  inline field_loose *alloc_loose(const tag_t tag, unsigned units);
  inline void release_loose(field_loose *loose, unsigned units);
  inline field_loose *
  release_data(relative_payload *chunk, unsigned units,
               const std::pair<field_loose *, field_loose *> &before_after,
               field_loose *hole0);
  void release_data(relative_payload *chunk, unsigned units) {
    assert(units > 0);
    release_data(chunk, units,
                 lookup_adjacent_holes(chunk->flat, chunk->flat + units),
                 nullptr);
  }
  inline relative_payload *realloc_data(relative_offset &ref, unsigned have,
                                        unsigned needed);
  inline void compactify(onstack_allocation_arena &onstack_arena);
  inline bool sort(onstack_allocation_arena &onstack_arena);
};

static inline cxx14_constexpr field_loose *
best_fit(unsigned units, field_loose *left, field_loose *right) {
  constexpr_assert(units > 0 && right != nullptr);
  if (left == nullptr)
    return right;
  if (right == nullptr)
    return left;

  /* если меньше лишнего места или ближе к началу кортежа при том-же размере */
  unsigned excess_left = left->hole_get_units() - units;
  unsigned excess_right = right->hole_get_units() - units;
  if (excess_left < excess_right)
    return left;
  if (excess_left > excess_right)
    return right;

  return (left->hole_begin() < right->hole_begin()) ? left : right;
}

template <gear::hole_search_mode MODE>
__pure_function field_loose *gear::lookup_hole(unsigned units) noexcept {
  if (junk_.count < 1 || junk_.volume < units)
    return nullptr;

  if_constexpr(MODE == hole_search_mode::exactly_size) return const_cast<
      field_loose *>(
      lookup(is_sorted(), begin_index(), end_index(), make_hole(units)));
  else if (0 == /* нет смысла искать, так как дырка ненулевого размера потребует
              усечения и поиска еще одной дырки... */
           units) return const_cast<field_loose *>(lookup(is_sorted(),
                                                          begin_index(),
                                                          end_index(),
                                                          make_hole(units)));
  else {
    field_loose *found = nullptr;
    std::size_t left_holes = junk_.count;
    /* TODO: SIMD */
    for (field_loose *scan = begin_index(); left_holes > 0; ++scan) {
      assert(scan != end_index());
      if (!scan->is_hole())
        continue;
      if (scan->hole_get_units() >= units) {
        if_constexpr(MODE == hole_search_mode::any_suitable) return scan;
        /* MODE == hole_search_mode::best_fit */
        found = best_fit(units, found, scan);
      }
      if (--left_holes == 0)
        break;
    }
    return found;
  }
}

std::pair<field_loose *, field_loose *>
gear::lookup_adjacent_holes(const unit_t *chunk_begin,
                            const unit_t *chunk_end) noexcept {
  assert(chunk_begin < chunk_end);
  std::size_t left_holes = junk_.count;
  field_loose *before = nullptr, *after = nullptr;

  /* TODO: SIMD */
  for (field_loose *scan = begin_index(); left_holes > 0; ++scan) {
    assert(scan != end_index());
    if (!scan->is_hole())
      continue;
    if (scan->hole_get_units()) {
      after = (chunk_end == scan->hole_begin()) ? scan : after;
      before = (scan->hole_end() == chunk_begin) ? scan : before;
    }
    left_holes -= 1;
  }
  return std::make_pair(before, after);
}

void gear::trim_hole(const field_loose *hole) noexcept {
  while (true) {
    assert(hole == begin_index() && hole->is_hole() && junk_.count > 0 &&
           head_ < pivot_);
    assert(hole->hole_get_units() == 0 ||
           hole->hole_begin() == end_data_units());

    /* удаляем использованныую дырку из индекса */
    junk_.count -= 1;
    head_ += 1;

    /* если следом в в начале индекса еще одна пустая дырка, либо дырка
     * примыкающая к концу распределенного участка, то удаляем и её */
    if (head_ == pivot_)
      return;
    hole = begin_index();
    if (likely(!hole->is_hole()))
      return;

    const unsigned next_hole_units = hole->hole_get_units();
    if (next_hole_units == 0) {
      /* дырка нулевого размера, таких может быть несколько подряд */
      continue;
    }

    /* дыра ненулевого размера, такие не должны идти подряд, но могу
     * перемежаться дырками нулевого размера. */
    if (likely(hole->hole_end() != end_data_units()))
      return;

    assert(junk_.volume >= next_hole_units &&
           tail_ >= pivot_ + next_hole_units);
    assert(junk_.count > 0 && head_ < pivot_);
    /* удаляем, возвращая место в нераспредененное пространство */
    junk_.volume -= uint16_t(next_hole_units);
    tail_ -= next_hole_units;
  }
}

unit_t *gear::tail_alloc(unsigned units) {
  const std::size_t avail = tail_space_units();
  if (unlikely(avail < units))
    throw_insufficient_space(0, units);
  unit_t *ptr = &area_[tail_];
  tail_ += units;
  return ptr;
}

field_loose *gear::index_alloc(size_t notify_data_space) {
  if (junk_.count > 0) {
    /* пробуем найти в индексе дыру без связанного зазора в данных */
    field_loose *hole = const_cast<field_loose *>(
        lookup(is_sorted(), begin_index(), end_index(), make_hole(0)));
    if (hole) {
      junk_.count -= 1;
      return hole;
    }
  }

  /* проверяем наличие места и двигаем голову индекса */
  if (unlikely(head_space() < 1))
    throw_insufficient_space(1, notify_data_space);
  return erthink::constexpr_pointer_cast<field_loose *>(&area_[--head_]);
}

relative_payload *gear::alloc_data(unsigned units, field_loose *const hole) {
  if (hole == nullptr)
    return erthink::constexpr_pointer_cast<relative_payload *>(
        tail_alloc(units));

  assert(junk_.volume >= units && junk_.count >= 1);
  junk_.volume -= uint16_t(units);
  unsigned excess = hole->hole_get_units() - units;
  relative_payload *chunk = hole->relative.payload();
  if (likely(excess == 0)) {
    /* дырка использована полностью */
    hole->hole_purge();
    /* если дырка первая в индексе, то её нужно удалить и проверить
     * возможность удаления последующей дырки */
    if (hole == begin_index())
      trim_hole(hole);
  } else {
    /* дырка использована не полностью */
    hole->relative.add_delta(units);
    hole->hole_set_units(excess);
  }

  mark_unsorted();
  return chunk;
}

field_loose *gear::alloc_loose(const tag_t tag, unsigned units) {
  if (units == 0) {
    assert(is_inplaced(tag));
    field_loose *loose = index_alloc(0);
    loose->genius_and_id = uint16_t(tag);
    loose->relative.reset_payload();
    mark_unsorted();
    return loose;
  }

  assert(units < bytes2units(max_tuple_bytes_netto));
  assert(!is_inplaced(tag));
  field_loose *hole = lookup_hole<hole_search_mode::best_fit>(units);
  if (hole == nullptr) {
    if (unlikely(tail_space_units() < units))
      throw_insufficient_space(1, units);
    hole = index_alloc(units);
    hole->genius_and_id = uint16_t(tag);
    hole->relative.set_payload(&area_[tail_]);
    tail_ += units;
    mark_unsorted();
    return hole;
  }

  assert(junk_.volume >= units && junk_.count >= 1);
  unsigned excess = hole->hole_get_units() - units;
  if (excess == 0) {
    /* дырка использована полностью */
    junk_.volume -= uint16_t(units);
    junk_.count -= 1;
    hole->genius_and_id = uint16_t(tag);
    mark_unsorted();
    return hole;
  }

  /* дырка использована не полностью */
  field_loose *loose = index_alloc(units);
  loose->genius_and_id = uint16_t(tag);
  loose->relative.set_payload(hole->hole_begin());
  hole->relative.add_delta(excess);
  hole->hole_set_units(excess);
  junk_.volume -= uint16_t(units);
  mark_unsorted();
  return loose;
}

field_loose *gear::merge_holes(field_loose *first,
                               field_loose *second) noexcept {
  assert(first->hole_get_units() && second->hole_get_units());
  assert(first->hole_end() == second->hole_begin());
  const unsigned units = first->hole_get_units() + second->hole_get_units();
  if (first != begin_index()) {
    /* вливаем вторую в первую, чтобы убрать из индекса вторую дырку, на случай
     * если она в начале индекса */
    first->hole_set_units(units);
    second->hole_purge();
    if (second == begin_index())
      trim_hole(second);
    return first;
  } else {
    /* вливаем первую во вторую,
     * чтобы убрать из начала индекса первую дырку */
    second->relative.sub_delta(first->hole_get_units());
    second->hole_set_units(units);
    first->hole_purge();
    trim_hole(first);
    return second;
  }
}

void gear::release_loose(field_loose *loose, unsigned units) {
  junk_.count += 1;
  if (units) {
    /* дыра с данными, т.е. НЕ только в индексе */
    loose->hole_set_units(units);

    /* добавляем в счетчики, чтобы прошли проверки внутри merge и trim */
    junk_.volume += uint16_t(units);

    const auto before_after = lookup_adjacent_holes(loose);
    /* объединяем с соседними если они есть */
    if (before_after.first)
      loose = merge_holes(before_after.first, loose);
    if (before_after.second)
      loose = merge_holes(loose, before_after.second);

    if (loose->hole_end() != end_data_units()) {
      /* дырка НЕ примыкает к концу данных, больше сделать ничего нельзя */
      return;
    }

    /* (!) получаем размер С УЧЕТОМ слияния дыр */
    units = loose->hole_get_units();
    /* дырка примыкает к концу данных, возвращем место в нераспределенное */
    tail_ -= units;
    junk_.volume -= uint16_t(units);
    /* далее обнуляем размер дыры */
  }

  /* дыра только в индексе, т.е. без данных */
  loose->hole_purge();
  if (loose == begin_index())
    trim_hole(loose);
}

field_loose *
gear::release_data(relative_payload *chunk, unsigned units,
                   const std::pair<field_loose *, field_loose *> &before_after,
                   field_loose *hole0) {
  assert(units > 0);
  assert(!hole0 || hole0->hole_get_units() == 0);

  if (chunk->flat + units == end_data_units()) {
    /* кусок примыкает к концу распределенных данных,
     * просто возвращем место в нераспределенное */
    tail_ -= units;
    if (before_after.first) {
      /* если есть предыдущая дырка, то она теперь примыкает к новому концу
       * данных */
      assert(before_after.second == nullptr &&
             before_after.first->hole_end() == end_data_units());
      units = before_after.first->hole_get_units();
      tail_ -= units;
      junk_.volume -= uint16_t(units);
      before_after.first->hole_purge();
      if (before_after.first == begin_index())
        trim_hole(before_after.first);
    }
    return nullptr;
  }

  /* добавляем в счетчики, чтобы прошли проверки внутри merge и trim */
  junk_.volume += uint16_t(units);

  field_loose *merged = nullptr;
  if (before_after.first) {
    assert(chunk->flat == before_after.first->hole_end());
    merged = before_after.first;
    merged->hole_set_units(units + merged->hole_get_units());
  }
  if (before_after.second) {
    assert(chunk->flat + units == before_after.second->hole_begin());
    if (merged)
      merged = merge_holes(merged, before_after.second);
    else {
      merged = before_after.second;
      merged->relative.sub_delta(units);
      merged->hole_set_units(units + merged->hole_get_units());
    }
  }

  if (merged == nullptr) {
    /* не нашлось соседних дырок для объединения с куском */
    assert(chunk->flat + units != end_data_units());
    /* кусок НЕ может примыкать к концу данных, оформляем дыркой */
    if (hole0 == nullptr) {
      hole0 = index_alloc(0);
      junk_.count += 1;
    }
    hole0->hole_set_units(units);
    hole0->relative.set_payload(chunk);
    return hole0;
  }
  /* кусок был слит с соседней дыркой */

  assert(merged->hole_end() != end_data_units());
  /* дырка НЕ может примыкать к концу данных */
  return merged;
}

relative_payload *gear::realloc_data(relative_offset &ref, const unsigned have,
                                     const unsigned needed) {
  assert(have != needed);
  assert(have > 0 && have < UINT16_MAX);
  assert(needed > 0 && needed < UINT16_MAX);

  /* Здесь возможны три конкурирующие тактики:
   *  1. Корректность при вбросе исключений из-за нехватки места:
   *     - сначала запрашиваем новое место, затем освобождаем старое.
   *     - в краткосрочной перспективе приводит к опуханию кортежа, а также
   *       к ложным отказам при увеличении размера полей.
   *  2. Минимизация размера и фрагментации:
   *     - сначала освобождаем текущее, затем выделяем новое.
   *     - утрачивается текущее значение поля при невозможности выделить место.
   *       однако, это не является проблемой, если исключение будет перехвачено
   *       и операция присваивания будет повторена после пере-выделения буфера.
   *  3. Минимизация действий:
   *     - пытаемся не делать лишнего в некоторых частных случаях, но стратегия
   *       быстро вырождается, ибо таких случаев очень мало.
   *
   *  Реализованная тактика:
   *   - с минимальным количеством проверок идём от третьей "ленивой" тактики
   *     к первой "безопасной".
   *   - иногда без-затратно работает третья тактика, в основном вторая,
   *     и первая в опасных случаях (при нехватке места).
   */

  if (junk_.volume == 0) {
    /* Если в кортеже нет зазоров/дыр, то возможны только три случая, которые
     * можно обработать по-быстрому без поиска и других лишних действий. */
    if (ref.payload()->flat + have == end_data_units()) {
      /* Внутри нет зазоров и место выделено в самом конце. Поэтому пытаемся
       * непосредственно изменить размер выделенного места. Это одновременно
       * самый быстрый и единственно возможный вариант действий. */
      if (have < needed && unlikely(tail_space_units() < needed - have))
        throw_insufficient_space(0, needed);
      tail_ = tail_ - have + needed;
      return ref.payload();
    }

    if (needed > have) {
      /* Внутри нет зазоров и места требуется БОЛЬШЕ чем есть. В этом случае
       * нет других вариантов, кроме как сначала выделить новое место,
       * а затем освободить старое. */
      if (unlikely(tail_space_units() < needed ||
                   head_space() + junk_.count < 1))
        throw_insufficient_space(1, needed);
        /* Далее исключений быть не должно */
#ifndef NDEBUG
      try {
#endif
        field_loose *hole = index_alloc(needed);
        hole->hole_set_units(have);
        hole->relative.set_payload(ref.payload());
        junk_.count += 1;
        junk_.volume += uint16_t(have);
        ref.set_payload(tail_alloc(needed));
#ifndef NDEBUG
      } catch (const insufficient_space &) {
        assert(false);
        throw;
      }
#endif
      return ref.payload();
    }

    /* Внутри нет зазоров и места требуется МЕНЬШЕ чем есть. В этом случае
     * нет других вариантов, кроме как добавить новую дыру и поместить в нее
     * лишнее место. */
    assert(needed < have);
    field_loose *hole =
        index_alloc(needed) /* Тут может выскочить исключение */;
    hole->hole_set_units(have - needed);
    hole->relative.set_payload(ref.payload()->flat + needed);
    junk_.count += 1;
    junk_.volume += uint16_t(have - needed);
    return ref.payload();
  }

  /* Внутри есть зазоры, поэтому для уменьшения фрагментации следует пытаться
   * объединять их с освобождаемым местом. Следовательно, поиск соседних дырок
   * потребуется в любом случае и не будет напрасным действием. */
  const auto before_after =
      lookup_adjacent_holes(ref.payload()->flat, ref.payload()->flat + have);
  unsigned adjacent_space =
      (before_after.first ? before_after.first->hole_get_units() : 0) +
      (before_after.second ? before_after.second->hole_get_units() : 0);

  /* Выгоднее сначала освободить прежнее место и объединить дырки, а затем
   * выделять новое место. Однако, такой порядок действий безопасен только
   * когда ресурсов достаточно и выделение после освобождения НЕ приведёт
   * к выбросу исключения "посередине". */

  /* сюда будет сохранен результат, если для проверки возможности добавления
   * дырок придёться искать в индексе пустую, т.е. чтобы не искать повторно */
  field_loose *hole4release = nullptr;

  /* сюда будет сохранен результат, если для проверки возможности выделения
   * места придёться искать подходящую дырку, т.е. чтобы не искать повторно */
  field_loose *hole4alloc = nullptr;

  /* УВЕЛИЧЕНИЕ будет успешным если объединение дыр даёт достаточно места. */
  if (have < needed && have + adjacent_space >= needed) {
  release_and_alloc:
    /* Далее исключений быть не должно! */
#ifndef NDEBUG
    try {
#endif
      hole4release =
          release_data(ref.payload(), have, before_after, hole4release);

      if (hole4alloc /* Если ранее была найдена подходящая дыра для выделения
                        места, но теперь нужно проверить что она сохранилась. */
          && hole4alloc >= begin_index() && hole4alloc->hole_get_units()) {
        assert(hole4alloc < end_index());
        assert(hole4alloc->hole_get_units() >= needed);
        assert(hole4alloc->hole_begin() >= begin_data_units() &&
               hole4alloc->hole_end() < end_data_units());
        /* Похоже что дыра сохранилась. Выбираем наиболее подходящее между этой
         * дырой и потенциальной новой, которая могла быть создана при
         * освобождении (с учётом слияний). */
        hole4alloc = best_fit(needed, hole4release, hole4alloc);
        assert(hole4alloc == lookup_hole<hole_search_mode::best_fit>(needed));
      } else
        hole4alloc = lookup_hole<hole_search_mode::best_fit>(needed);

      ref.set_payload(alloc_data(needed, hole4alloc));
#ifndef NDEBUG
    } catch (const insufficient_space &) {
      assert(false);
      throw;
    }
#endif
    return ref.payload();
  }

  const bool can_add_a_hole =
      adjacent_space > /* место в индексе появится в результате слияния */ 0 ||
      head_space() > 0 ||
      nullptr !=
          (hole4release = lookup_hole<hole_search_mode::exactly_size>(0));

  if (unlikely(!can_add_a_hole)) {
    /* Нет возможности добавить новую дыру или использовать соседние. Остаётся
     * два варианта:
     *  - найти точно подходящую по размеру дырку и "обменяться" с ней.
     *  - иначе, если место выделено в самом конце (примыкает к невыделенному
     *    пространству), то изменить его границу подвинув "хвост". */
    hole4alloc = lookup_hole<hole_search_mode::exactly_size>(needed);
    if (hole4alloc) {
      assert(needed == hole4alloc->hole_get_units());

      /* обмениваемся кусками с найденной дырой */
      unit_t *const chunk = ref.payload()->flat;
      ref.set_payload(hole4alloc->hole_begin());
      hole4alloc->relative.set_payload(chunk);
      hole4alloc->hole_set_units(have);
      junk_.volume += uint16_t(have - needed);

      /* если дырка примыкает к концу данных, то её нужно "испарить" */
      if (chunk == end_data_units()) {
        /* возвращем место в нераспределенное */
        tail_ -= have;
        junk_.volume -= uint16_t(have);
        /* обнуляем размер дыры */
        hole4alloc->hole_purge();
        if (hole4alloc == begin_index())
          trim_hole(hole4alloc);
      }
      return ref.payload();
    }

    if (ref.payload()->flat + have == end_data_units()) {
      /* Место выделено в самом конце, пытаемся непосредственно
       * изменить его размер двигая "хвост". */
      if (have < needed && unlikely(tail_space_units() < needed - have))
        throw_insufficient_space(1, needed);
      tail_ = tail_ - have + needed;
      return ref.payload();
    }

    throw_insufficient_space(1, needed);
  }
  /* Есть возможность добавить новую дыру или использовать соседние. */
  assert(can_add_a_hole);

  /* УВЕЛИЧЕНИЕ будет успешным если достаточно нераспределенного места
   * и есть возможность добавления дырки. В свою очередь, для УМЕНЬШЕНИЯ
   * достаточно только добавления дырки в индекс. */
  const bool is_space_enough = have > needed || needed <= tail_space_units();
  if (is_space_enough)
    goto release_and_alloc;

  /* Не достаточно нераспределенного места, нужно искать подходящую дыру.
   * Если такая дыра есть, то выделение места будет успешным. */
  hole4alloc = lookup_hole<hole_search_mode::best_fit>(needed);
  if (likely(hole4alloc))
    goto release_and_alloc;

  throw_insufficient_space(0, needed);
}

//------------------------------------------------------------------------------

static inline void overlapped_copy(unit_t *dst, const unit_t *src,
                                   std::size_t count) {
  assert(dst < src && count > 0);
  count -= 1;
  dst += count;
  src += count;
  switch (count) {
  default:
    std::memmove(dst - count, src - count, (count + 1) * sizeof(unit_t));
    return;
#define HERE_CASE_ITEM(N)                                                      \
  case N:                                                                      \
    dst[-N] = src[-N]

    HERE_CASE_ITEM(15);
    __fallthrough;
    HERE_CASE_ITEM(14);
    __fallthrough;
    HERE_CASE_ITEM(13);
    __fallthrough;
    HERE_CASE_ITEM(12);
    __fallthrough;
    HERE_CASE_ITEM(11);
    __fallthrough;
    HERE_CASE_ITEM(10);
    __fallthrough;
    HERE_CASE_ITEM(9);
    __fallthrough;
    HERE_CASE_ITEM(8);
    __fallthrough;
    HERE_CASE_ITEM(7);
    __fallthrough;
    HERE_CASE_ITEM(6);
    __fallthrough;
    HERE_CASE_ITEM(5);
    __fallthrough;
    HERE_CASE_ITEM(4);
    __fallthrough;
    HERE_CASE_ITEM(3);
    __fallthrough;
    HERE_CASE_ITEM(2);
    __fallthrough;
    HERE_CASE_ITEM(1);
    __fallthrough;
#undef HERE_CASE_ITEM
  case 0 /* allows gcc/clang to simplify switch-operator code */:
    *dst = *src;
  }
}

struct compact_item {
  uint16_t payload_offset;
  uint16_t length;
  uint32_t referrer_offset;

  cxx14_constexpr static uint16_t
  payload2offset(const unit_t *const basis,
                 const relative_offset &relative) noexcept {
    const ptrdiff_t offset = relative.payload()->flat - basis;
    constexpr_assert(offset > 0 && offset <= UINT16_MAX);
    return static_cast<uint16_t>(offset);
  }

  constexpr unit_t *payload(const unit_t *const basis) const noexcept {
    return const_cast<unit_t *>(basis + payload_offset);
  }

  cxx14_constexpr static uint32_t
  referrer2offset(const unit_t *const basis,
                  const relative_offset *relative) noexcept {
    const ptrdiff_t offset =
        erthink::constexpr_pointer_cast<const char *>(relative) -
        erthink::constexpr_pointer_cast<const char *>(basis);
    constexpr_assert(offset >= 0 && offset < fptu::max_tuple_bytes_netto);
    return static_cast<uint32_t>(offset);
  }

  constexpr relative_offset &referrer(const unit_t *const basis) const
      noexcept {
    return *erthink::constexpr_pointer_cast<relative_offset *>(
        const_cast<char *>(
            erthink::constexpr_pointer_cast<const char *>(basis)) +
        referrer_offset);
  }

  compact_item() noexcept = default;

  cxx14_constexpr compact_item(const unit_t *const basis,
                               const field_loose *const field) noexcept
      : payload_offset(payload2offset(basis, field->relative)),
        length(uint16_t(is_fixed_size(field->type())
                            ? loose_units_dynamic(field->type())
                            : field->stretchy_units())),
        referrer_offset(referrer2offset(basis, &field->relative)) {
    constexpr_assert(!field->is_hole() && !is_inplaced(field->type()) &&
                     field->relative.have_payload());
    const std::size_t length_units = is_fixed_size(field->type())
                                         ? loose_units_dynamic(field->type())
                                         : field->stretchy_units();
    constexpr_assert(length_units > 0 && length_units <= UINT16_MAX);
    (void)length_units;
  }

  cxx14_constexpr compact_item(const unit_t *const basis, const genus type,
                               const field_preplaced *const field) noexcept
      : payload_offset(payload2offset(basis, field->relative)),
        length(
            uint16_t(field->relative.payload()->stretchy.brutto_units(type))),
        referrer_offset(referrer2offset(basis, &field->relative)) {
    constexpr_assert(!is_fixed_size(type) && field->relative.have_payload());
    const std::size_t length_units =
        field->relative.payload()->stretchy.brutto_units(type);
    constexpr_assert(length_units > 0 && length_units <= UINT16_MAX);
    (void)length_units;
  }
};

using compact_vector =
    std::vector<compact_item, onstack_short_allocator<compact_item>>;

inline void gear::compactify(onstack_allocation_arena &onstack_arena) {
  /* ВАЖНО (на всякий случай, чтобы не забыть):
   *  - компактификация выполняется в ДВА ШАГА: удаление дырок из индекса,
   *    затем перемещение полезных данных.
   *  - эти ДВА ШАГА взаимосвязаны и не могут выполняться по-отдельности
   *    или в другом порядке! */

  int moveable_count = -1 /* счетчик потенциально перемещаемых элементов для
                               оценки требуемого размера вектора */
      ;

  /* LY: при необходимости сначала убираем дырки/зазоры из индекса */
  if (junk_.count) {
    debug_check();
    field_loose *src, *dst;
    /* копируем индекс пропуская дырки, одновременно подсчитываем перемещаемые
     * элементы для оценки размера вектора */
    moveable_count = 0;
    for (src = dst = end_index() - 1; src >= begin_index(); --src) {
      /* TODO: SIMD? */
      if (src->is_hole())
        continue;
      moveable_count +=
          !is_inplaced(src->type()) && src->relative.have_payload();
      if (src != dst) {
        dst->loose_header = src->loose_header;
        if (!is_inplaced(src->type()))
          dst->relative.sub_delta(dst - src);
      }
      --dst;
    }
    assert(dst - src == junk_.count);
    assert(dst + 1 == begin_index() + junk_.count);
    assert(src + 1 == begin_index());
    assert(dst > erthink::constexpr_pointer_cast<field_loose *>(area_) &&
           dst < end_index());
    head_ += junk_.count;
    junk_.count = 0;
    if (junk_.volume == 0)
      debug_check();
  }

  /* теперь при необходимости дефрагментируем данные */
  if (junk_.volume) {
    /* считаем требуемый размер вектора подсчитывая перемещаемые элементы */
    if (moveable_count < 0) {
      moveable_count = 0;
      /* TODO: SIMD */
      for (field_loose *loose = begin_index(); loose < end_index(); ++loose) {
        assert(!loose->is_hole());
        moveable_count +=
            !is_inplaced(loose->type()) && loose->relative.have_payload();
      }
    }
    if (schema_) {
      for (const token &ident : schema_->tokens()) {
        /* TODO: обеспечить сортировку токенов в схеме, чтобы preplaced были в
         * начале, а здесь ранний выход из цикла */
        if (ident.is_preplaced() && !is_fixed_size(ident.type())) {
          const field_preplaced *const preplaced =
              erthink::constexpr_pointer_cast<const field_preplaced *>(
                  begin_data_bytes() + ident.preplaced_offset());
          moveable_count += preplaced->relative.have_payload();
        }
      }
    }
    assert(moveable_count > 0);

    /* заполняем вектор описаниями чанков данных */
    compact_vector vector(onstack_arena);
    vector.reserve(moveable_count);
    const unit_t *const basis = &area_[head_];
    if (schema_) {
      for (const token &ident : schema_->tokens()) {
        if (ident.is_preplaced() && !is_fixed_size(ident.type())) {
          const field_preplaced *const preplaced =
              erthink::constexpr_pointer_cast<const field_preplaced *>(
                  begin_data_bytes() + ident.preplaced_offset());
          vector.emplace_back(basis, ident.type(), preplaced);
        }
      }
    }
    /* идём от конца к началу, чтобы максимально использовать естественный
     * порядок добавления полей */
    for (field_loose *loose = end_index(); --loose >= begin_index();) {
      assert(!loose->is_hole());
      if (!is_inplaced(loose->type()) && loose->relative.have_payload()) {
        vector.emplace_back(basis, loose);
      }
    }
    assert(vector.size() == (size_t)moveable_count);

    /* сортируем в порядке возрастания адресов/смещений */
    std::sort(vector.begin(), vector.end(),
              [](const compact_item &a, const compact_item &b) {
                return a.payload_offset < b.payload_offset;
              });

    /* перемещаем данные без зазоров корректируя ссылки */
    unit_t *dst = const_cast<unit_t *>(begin_data_units());
    if (schema_) {
      dst += schema_->preplaced_units();
      assert(dst < end_data_units());
    }
    for (const auto &chunk : vector) {
      const unit_t *const src = chunk.payload(basis);
      assert(src >= dst && src + chunk.length <= end_data_units());
      if (src != dst) {
        chunk.referrer(basis).sub_delta(src - dst);
        overlapped_copy(dst, src, chunk.length);
      }
      dst += chunk.length;
    }
    assert(dst == end_data_units() - junk_.volume);
    tail_ -= junk_.volume;
    junk_.volume = 0;
    debug_check();
  }
}

//------------------------------------------------------------------------------

struct sort_item {
  uint16_t genius_and_id;
  uint16_t inplaced_or_offset;

  cxx14_constexpr static uint16_t
  payload2offset(const unit_t *const basis,
                 const relative_offset &relative) noexcept {
    const ptrdiff_t offset = relative.payload()->flat - basis;
    constexpr_assert(offset > 0 && offset <= UINT16_MAX);
    return static_cast<uint16_t>(offset);
  }

  constexpr unit_t *payload(const unit_t *const basis) const noexcept {
    return const_cast<unit_t *>(basis + inplaced_or_offset);
  }

  sort_item() noexcept = default;

  cxx14_constexpr sort_item(const field_loose *const field,
                            const unit_t *const basis) noexcept
      : genius_and_id(field->genius_and_id),
        inplaced_or_offset(
            (is_inplaced(field->type()) || !field->relative.have_payload())
                ? field->inplaced
                : payload2offset(basis, field->relative)) {
    if (!is_inplaced(field->type()) && !field->relative.have_payload())
      assert(inplaced_or_offset == 0);
  }
};

using sort_vector = std::vector<sort_item, onstack_short_allocator<sort_item>>;

bool gear::sort(onstack_allocation_arena &onstack_arena) {
  debug_check();
  if (std::is_sorted(begin_index(), end_index(),
                     [](const field_loose &a, const field_loose &b) {
                       return a.genius_and_id > b.genius_and_id;
                     })) {
    mark_sorted();
    debug_check();
    return false;
  }

  sort_vector vector(index_size(), onstack_arena);
  const unit_t *const basis = &area_[head_];
  /* заполняем вектор */
  for (field_loose *loose = begin_index(); loose < end_index(); ++loose)
    vector.emplace_back(loose, basis);
  /* сортируем вектор по-убыванию, теперь дырки в начале вектора */
  std::sort(vector.begin(), vector.end(),
            [](const sort_item &a, const sort_item &b) {
              return a.genius_and_id > b.genius_and_id;
            });

  /* создаем индекс заново в отсортированном порядке */
  field_loose *loose = begin_index();
  for (const auto &chunk : vector) {
    loose->genius_and_id = chunk.genius_and_id;
    loose->inplaced = chunk.inplaced_or_offset;
    if (!is_inplaced(loose->type()) && chunk.inplaced_or_offset)
      loose->relative.set_payload(chunk.payload(basis));
    loose += 1;
  }
  assert(loose == end_index());
  assert(std::is_sorted(begin_index(), end_index(),
                        [](const field_loose &a, const field_loose &b) {
                          return a.genius_and_id > b.genius_and_id;
                        }));
  debug_check();

  mark_sorted();
  debug_check();
  return true;
}

} // namespace

//------------------------------------------------------------------------------

__hot relative_payload *tuple_rw::alloc_data(const std::size_t units) {
  return static_cast<class gear *>(this)->alloc_data(unsigned(units));
}

__hot void tuple_rw::release_data(relative_payload *chunk,
                                  const std::size_t units) {
  return static_cast<class gear *>(this)->release_data(chunk, unsigned(units));
}

__hot field_loose *tuple_rw::alloc_loose(const tag_t tag,
                                         const std::size_t units) {
  return static_cast<class gear *>(this)->alloc_loose(tag, unsigned(units));
}

__hot void tuple_rw::release_loose(field_loose *loose,
                                   const std::size_t units) {
  return static_cast<class gear *>(this)->release_loose(loose, unsigned(units));
}

__hot relative_payload *tuple_rw::realloc_data(relative_offset &ref,
                                               const std::size_t have,
                                               const std::size_t needed) {
  return static_cast<class gear *>(this)->realloc_data(ref, unsigned(have),
                                                       unsigned(needed));
}

__hot bool tuple_rw::optimize(const optimize_flags flags) {
  onstack_allocation_arena arena;

  bool interators_invalidated = false;
  if ((flags & optimize_flags::compactify) && junk_.both != 0) {
    interators_invalidated = junk_.count > 0;
    static_cast<class gear *>(this)->compactify(arena);
  }

  if (((flags & optimize_flags::sort_index) &&
       loose_count() >= sort_index_threshold && !is_sorted()) ||
      unlikely(flags & optimize_flags::force_sort_index)) {
    assert(arena.used() == 0);
    interators_invalidated |= static_cast<class gear *>(this)->sort(arena);
  }

  return interators_invalidated;
}

} // namespace details
} // namespace fptu

#include "fast_positive/details/warnings_pop.h"
