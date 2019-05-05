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

namespace fptu {

void tuple_rw_managed::manage_space_deficit(const insufficient_space &wanna) {
  assert(wanna.data_space > pimpl_->end_ - pimpl_->tail_ ||
         wanna.index_space > pimpl_->head_);
  debug_check();

  const size_t wanna_units = wanna.data_space + wanna.index_space;
  size_t junk_units = pimpl_->junk_.volume + pimpl_->junk_.count;
  size_t allocated_units = pimpl_->tail_ - pimpl_->head_;
  size_t unallocated_units = pimpl_->end_ - allocated_units;
  if (unlikely(allocated_units - junk_units + wanna_units >= UINT16_MAX ||
               wanna.index_space > fundamentals::max_fields ||
               wanna.data_space > fundamentals::max_tuple_bytes_netto)) {
    /* Запрос превышает максимальный размер. Чтобы не искажать картину бросаем
     * исключение не изменяя кортеж. */
    throw_tuple_overflow();
  }

  if (wanna_units <= junk_units + unallocated_units) {
    /* Можно обойтись компактификацией и/или перемещением, без выделения буфера
     * большего размера. Считаем это более успешной стратегией, так как буфер
     * под кортеж должен был быть выделен с умом и расчетом. */
    pimpl_->compactify();
    junk_units = pimpl_->junk_.volume + pimpl_->junk_.count;
    allocated_units = pimpl_->tail_ - pimpl_->head_;
    unallocated_units = pimpl_->end_ - allocated_units;
    assert(junk_units == 0 && unallocated_units >= wanna_units);
    ptrdiff_t shift = 0;
    const size_t index_used =
        pimpl_->pivot_ - pimpl_->head_ + wanna.index_space;
    const size_t data_used = pimpl_->tail_ - pimpl_->pivot_ + wanna.data_space;
    const size_t whole_used = index_used + data_used;
    if (wanna.index_space > pimpl_->head_) {
      /* Недостаточно места для индекса, перемещаем всё назад. Пытаемся
       * переместить "с запасом", распределяя свободное место пропорционально
       * актуальным размерам индекса (с учетом приращения) и данных. При этом
       * выполняем ceil-округление для места резервируемого под индекс. */
      const ptrdiff_t reserve =
          std::max(wanna.index_space,
                   std::min((unallocated_units * index_used + whole_used - 1) /
                                whole_used,
                            unallocated_units - wanna.data_space));
      shift = reserve - pimpl_->head_;
    } else if (wanna.data_space > pimpl_->end_ - pimpl_->tail_) {
      /* Недостаточно места для данных, перемещаем всё вперед. Пытаемся
       * переместить "с запасом", распределяя свободное место пропорционально
       * актуальным размерам индекса и данных (с учетом приращения). При этом
       * выполняем floor-округление для места резервируемого под данные. */
      const ptrdiff_t reserve = std::max(
          wanna.data_space, std::min(unallocated_units * data_used / whole_used,
                                     unallocated_units - wanna.index_space));
      shift = -(reserve - ptrdiff_t(pimpl_->end_ - pimpl_->tail_));
    }

    assert(pimpl_->tail_ + ptrdiff_t(wanna.data_space) + shift <=
           ptrdiff_t(pimpl_->end_));
    assert(pimpl_->head_ - ptrdiff_t(wanna.index_space) + shift >= 0);
    if (shift) {
      auto src = &pimpl_->area_[pimpl_->head_];
      auto dst = &pimpl_->area_[pimpl_->head_ + shift];
      std::memmove(dst, src,
                   details::units2bytes(pimpl_->tail_ - pimpl_->head_));
      pimpl_->head_ += int(shift);
      pimpl_->pivot_ += int(shift);
      pimpl_->tail_ += int(shift);
      debug_check();
    }
    return;
  }

  /* Места точно не хватает - необходимо выделять новый буфер, копировать данные
   * и освобождать текущий. Это очень дорогая операция, поэтому теперь действуем
   * с большим запасом:
   *   - новый буфер выделяем по следующей степени масштаба,
   *     это приведёт к увеличению до 4-х раз (но возможно гораздо меньше).
   *   - недостающий ресурс (место под индекс и/или под данные) увеличиваем
   *     минимум вдвое.
   *
   * Таким образом, недостающий ресурс будет удвоен, а размер нового буфера
   * будет размером из "линейки масштабов". */

  if (allocated_units + wanna_units >= UINT16_MAX ||
      junk_units + junk_units > allocated_units) {
    /* Без компактификации расширение на запрошенное место превысит предел
     * размера кортежа.
     * Либо если в кортеже много (половина или больше) дырок, то перед
     * копированием выгоднее выполнить компактификацию. */
    pimpl_->compactify();
  }

  const size_t data_capacity_units = size_t(pimpl_->end_ - pimpl_->pivot_);
  /* Место под данные: удваиваем если не хватило, иначе просим чуть больше.
   * Однако, под данные отойдет всё место после резервирования под индекс.
   * Поэтому, с учетом перехода на следующий уровень масштаба, место под данные
   * скорее всего будет выделено примерно в четверо больше. */
  const size_t growth_data = std::min(
      size_t(fundamentals::max_tuple_bytes_netto),
      (wanna.data_space > pimpl_->tail_space_units())
          ? details::units2bytes((wanna.data_space + data_capacity_units) * 2)
          : details::units2bytes(data_capacity_units + 42));

  /* Место под индекс: удваиваем если не хватило или использовано хотя-бы на
   * половину, иначе оставляем как есть. */
  const size_t growth_index =
      std::min(size_t(fundamentals::max_fields),
               (wanna.index_space > pimpl_->head_space() ||
                pimpl_->head_ + pimpl_->head_ < pimpl_->pivot_)
                   ? (wanna.index_space + pimpl_->pivot_) * 2
                   : pimpl_->pivot_);

  size_t growth_space = details::tuple_rw::estimate_required_space(
      growth_index, growth_data, pimpl_->schema_);
  hippeus::buffer *old_buffer = pimpl_->get_buffer();
  if (unlikely(!old_buffer)) {
    /* Буфера нет (buffer_offset_ равен нулю). Кортеж был создан через
     * legacy C-API. Перевыделяем память через realloc(). */
    assert(growth_space % fundamentals::unit_size == 0);
    void *const new_legacy_buffer =
        std::realloc(static_cast<void *>(pimpl_), growth_space);
    if (!new_legacy_buffer)
      throw std::bad_alloc();
    pimpl_ =
        erthink::constexpr_pointer_cast<details::tuple_rw *>(new_legacy_buffer);
    pimpl_->end_ = unsigned(std::min(
        ptrdiff_t(max_tuple_units_netto),
        erthink::constexpr_pointer_cast<details::unit_t *>(new_legacy_buffer) +
            growth_space % fundamentals::unit_size - pimpl_->area_));
    debug_check();

    /* Память перевыделенна. Теперь должно быть достаточно места для данных, но
     * для расширения места под индекс потроха кортежа нужно передвинуть. */
    assert(wanna.data_space >= pimpl_->end_ - pimpl_->tail_);
    assert(wanna_units >= pimpl_->end_ - pimpl_->tail_ + pimpl_->head_);
    if (growth_index > pimpl_->pivot_) {
      /* Недостаточно места для индекса, перемещаем всё назад */
      const ptrdiff_t shift = growth_index - pimpl_->pivot_;

      assert(pimpl_->tail_ + shift <= ptrdiff_t(pimpl_->end_));
      auto src = &pimpl_->area_[pimpl_->head_];
      auto dst = &pimpl_->area_[pimpl_->head_ + shift];
      std::memmove(dst, src,
                   details::units2bytes(pimpl_->tail_ - pimpl_->head_));
      pimpl_->head_ += int(shift);
      pimpl_->pivot_ += int(shift);
      pimpl_->tail_ += int(shift);
      debug_check();
    }
    return;
  }

  assert(old_buffer->space < growth_space);
  for (int scale = initiation_scale::tiny; scale <= initiation_scale::extreme;
       ++scale) {
    const size_t granulated_size =
        estimate_space_for_tuple(initiation_scale(scale), pimpl_->schema_);
    if (granulated_size >= growth_space) {
      growth_space = granulated_size;
      break;
    }
  }
  assert(old_buffer->space < growth_space);

  hippeus::buffer_ptr holder(
      hippeus::buffer::borrow(old_buffer->host, growth_space, true));
  details::tuple_rw *new_tuple = new (holder->data())
      details::tuple_rw(details::tuple_rw::get_buffer_offset(holder.get()),
                        holder->size(), growth_index, pimpl_->schema_);

  new_tuple->head_ -= pimpl_->pivot_ - pimpl_->head_;
  new_tuple->tail_ += pimpl_->tail_ - pimpl_->pivot_;
  std::memcpy(static_cast<void *>(new_tuple->begin_index()),
              pimpl_->begin_index(),
              details::units2bytes(pimpl_->tail_ - pimpl_->head_));
  new_tuple->debug_check();

  holder.release();
  pimpl_ = new_tuple;
  holder.reset(old_buffer);
  debug_check();
}

#define HERE_THUNK_MAKE(VALUE_TYPE, NAME)                                      \
  void tuple_rw_managed::set_##NAME(const token &ident,                        \
                                    const VALUE_TYPE value) {                  \
    while (true /* может быть повторное исключение */) {                       \
      try {                                                                    \
        return base::set_##NAME(ident, value);                                 \
      } catch (const insufficient_space &deficit) {                            \
        manage_space_deficit(deficit);                                         \
        continue;                                                              \
      }                                                                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  details::tuple_rw::iterator_rw<token> tuple_rw_managed::insert_##NAME(       \
      const token &ident, const VALUE_TYPE value) {                            \
    while (true /* может быть повторное исключение */) {                       \
      try {                                                                    \
        return base::insert_##NAME(ident, value);                              \
      } catch (const insufficient_space &deficit) {                            \
        manage_space_deficit(deficit);                                         \
        continue;                                                              \
      }                                                                        \
    }                                                                          \
  }

HERE_THUNK_MAKE(tuple_ro_weak &, nested)
HERE_THUNK_MAKE(string_view &, string)
HERE_THUNK_MAKE(std::string &, string)
HERE_THUNK_MAKE(string_view &, varbinary)
HERE_THUNK_MAKE(property_pair &, property)
HERE_THUNK_MAKE(bool, bool)
HERE_THUNK_MAKE(short, enum)
HERE_THUNK_MAKE(int8_t, i8)
HERE_THUNK_MAKE(uint8_t, u8)
HERE_THUNK_MAKE(int16_t, i16)
HERE_THUNK_MAKE(uint16_t, u16)
HERE_THUNK_MAKE(int32_t, i32)
HERE_THUNK_MAKE(uint32_t, u32)
HERE_THUNK_MAKE(int64_t, i64)
HERE_THUNK_MAKE(uint64_t, u64)
HERE_THUNK_MAKE(float, f32)
HERE_THUNK_MAKE(double, f64)
HERE_THUNK_MAKE(decimal64, decimal)
HERE_THUNK_MAKE(datetime_t, datetime)
HERE_THUNK_MAKE(uuid_t &, uuid)
HERE_THUNK_MAKE(int128_t &, int128)
HERE_THUNK_MAKE(uint128_t &, uint128)
HERE_THUNK_MAKE(binary96_t &, bin96)
HERE_THUNK_MAKE(binary128_t &, bin128)
HERE_THUNK_MAKE(binary160_t &, bin160)
HERE_THUNK_MAKE(binary192_t &, bin192)
HERE_THUNK_MAKE(binary224_t &, bin224)
HERE_THUNK_MAKE(binary256_t &, bin256)
HERE_THUNK_MAKE(binary320_t &, bin320)
HERE_THUNK_MAKE(binary384_t &, bin384)
HERE_THUNK_MAKE(binary512_t &, bin512)
HERE_THUNK_MAKE(ip_address_t &, ip_address)
HERE_THUNK_MAKE(mac_address_t, mac_address)
HERE_THUNK_MAKE(ip_net_t &, ip_net)
HERE_THUNK_MAKE(int64_t, integer)
HERE_THUNK_MAKE(uint64_t, integer)
HERE_THUNK_MAKE(int64_t, unsigned)
HERE_THUNK_MAKE(uint64_t, unsigned)
HERE_THUNK_MAKE(double, float)
#undef HERE_THUNK_MAKE

bool tuple_rw_managed::erase(const dynamic_iterator_rw &it) {
  /* Места может не хватить только при удалении preplaced полей,
   * из-за необходимости добавления "дырки" в индекс.
   * Поэтому loose-поля можно удалять без try/catch страховки. */
  if (likely(it.token().is_loose()))
    return base::erase(it);

  const cast_preplaced<token> preplaced_token(it.token());
  while (true) {
    try {
      /* После расширения буфера у R/W-представления кортежа и preplaced поля
       * будет другой адрес. Но для обхода всех проблем достаточно удалять
       * поле через шаблонный метод, передав ему preplaced-токен. */
      return get_impl()->remove(preplaced_token);
    } catch (const insufficient_space &deficit) {
      manage_space_deficit(deficit);
      /* повторного исключения быть не может, но так проще и меньше кода */
      continue;
    }
  }
}

bool tuple_rw_managed::erase(const token &ident) {
  /* Места может не хватить только при удалении preplaced полей,
   * из-за необходимости добавления "дырки" в индекс.
   * Поэтому loose-поля можно удалять без try/catch страховки. */
  if (likely(ident.is_loose()))
    return base::erase(ident);

  const cast_preplaced<token> preplaced_token(ident);
  while (true) {
    try {
      /* После расширения буфера у R/W-представления кортежа и preplaced поля
       * будет другой адрес. Но для обхода всех проблем достаточно удалять
       * поле через шаблонный метод, передав ему preplaced-токен. */
      return get_impl()->remove(preplaced_token);
    } catch (const insufficient_space &deficit) {
      manage_space_deficit(deficit);
      /* повторного исключения быть не может, но так проще и меньше кода */
      continue;
    }
  }
}

std::size_t tuple_rw_managed::erase(const dynamic_iterator_rw &from,
                                    const dynamic_iterator_rw &to) {
  assert(from.token() == to.token());
  if (from.token().is_loose())
    /* Места может не хватить только при удалении preplaced полей,
     * из-за необходимости добавления "дырки" в индекс.
     * Поэтому loose-поля можно удалять без try/catch страховки. */
    return base::erase(from, to);

  /* Preplaced-поля не могут повторяться. Поэтому в самом крайнем случае
   * здесь может быть только одно поле, которое проще удалить непосредственно
   * через токен. */
  assert(from.token().is_preplaced() && std::next(from) == to);

  const cast_preplaced<token> preplaced_token(from.token());
  while (true) {
    try {
      /* После расширения буфера у R/W-представления кортежа и preplaced поля
       * будет другой адрес. Но для обхода всех проблем достаточно удалять
       * поле через шаблонный метод, передав ему preplaced-токен. */
      return get_impl()->remove(preplaced_token) ? 1 : 0;
    } catch (const insufficient_space &deficit) {
      manage_space_deficit(deficit);
      /* повторного исключения быть не может, но так проще и меньше кода */
      continue;
    }
  }
}

std::size_t tuple_rw_managed::erase(const dynamic_collection_rw &collection) {
  return erase(const_cast<dynamic_collection_rw &>(collection).begin(),
               const_cast<dynamic_collection_rw &>(collection).end());
}

} // namespace fptu
