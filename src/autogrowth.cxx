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

void tuple_rw_managed::expand_underlying_buffer(
    const insufficient_space &deficit) {
  FPTU_NOT_IMPLEMENTED();
  (void)deficit;
}

#define HERE_THUNK_MAKE(VALUE_TYPE, NAME)                                      \
  void tuple_rw_managed::set_##NAME(const token &ident,                        \
                                    const VALUE_TYPE value) {                  \
    while (true /* может быть повторное исключение */) {                       \
      try {                                                                    \
        return base::set_##NAME(ident, value);                                 \
      } catch (const insufficient_space &deficit) {                            \
        expand_underlying_buffer(deficit);                                     \
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
        expand_underlying_buffer(deficit);                                     \
        continue;                                                              \
      }                                                                        \
    }                                                                          \
  }

HERE_THUNK_MAKE(tuple_ro_weak &, nested)
HERE_THUNK_MAKE(string_view &, string)
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
HERE_THUNK_MAKE(binary96_t &, b96)
HERE_THUNK_MAKE(binary128_t &, b128)
HERE_THUNK_MAKE(binary160_t &, b160)
HERE_THUNK_MAKE(binary192_t &, b192)
HERE_THUNK_MAKE(binary224_t &, b224)
HERE_THUNK_MAKE(binary256_t &, b256)
HERE_THUNK_MAKE(binary320_t &, b320)
HERE_THUNK_MAKE(binary384_t &, b384)
HERE_THUNK_MAKE(binary512_t &, b512)
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
   * из-за необходимости добавления "дырки" в индекс
   * Поэтому loose-поля можо удалять без страховки.*/
  if (likely(it.token().is_loose()))
    return base::erase(it);

  const cast_preplaced<token> preplaced_token(it.token());
  while (true /* может быть повторное исключение */) {
    try {
      assert(preplaced_token.is_preplaced());
      /* После расширения буфера у R/W-представдения кортежа и preplaced поля
       * будет другой адрес. Но для обхода всех проблем достаточно удалять
       * поле через шаблонный метод, передав ему preplaced-токен. */
      return get_impl()->remove(preplaced_token);
    } catch (const insufficient_space &deficit) {
      expand_underlying_buffer(deficit);
      continue;
    }
  }
}

bool tuple_rw_managed::erase(const token &ident) {
  /* Места может не хватить только при удалении preplaced полей,
   * из-за необходимости добавления "дырки" в индекс
   * Поэтому loose-поля можо удалять без страховки.*/
  if (likely(ident.is_loose()))
    return base::erase(ident);

  const cast_preplaced<token> preplaced_token(ident);
  while (true /* может быть повторное исключение */) {
    try {
      assert(preplaced_token.is_preplaced());
      /* После расширения буфера у R/W-представдения кортежа и preplaced поля
       * будет другой адрес. Но для обхода всех проблем достаточно удалять
       * поле через шаблонный метод, передав ему preplaced-токен. */
      return get_impl()->remove(preplaced_token);
    } catch (const insufficient_space &deficit) {
      expand_underlying_buffer(deficit);
      continue;
    }
  }
}

std::size_t tuple_rw_managed::erase(const dynamic_iterator_rw &from,
                                    const dynamic_iterator_rw &to) {
  assert(from.token() == to.token());
  if (from.token().is_loose())
    /* Места может не хватить только при удалении preplaced полей,
     * из-за необходимости добавления "дырки" в индекс
     * Поэтому loose-поля можо удалять без страховки.*/
    return base::erase(from, to);

  assert(from.token().is_preplaced());
  /* Preplaced-поля не могут повторяться. Поэтому в самом крайнем случае
   * здесь может быть только одно поле, которое проще удалить непосредственно
   * через токен. */
  assert(std::next(from) == to);

  const cast_preplaced<token> preplaced_token(from.token());
  while (true /* может быть повторное исключение */) {
    try {
      assert(preplaced_token.is_preplaced());
      /* После расширения буфера у R/W-представдения кортежа и preplaced поля
       * будет другой адрес. Но для обхода всех проблем достаточно удалять
       * поле через шаблонный метод, передав ему preplaced-токен. */
      return get_impl()->remove(preplaced_token) ? 1 : 0;
    } catch (const insufficient_space &deficit) {
      expand_underlying_buffer(deficit);
      continue;
    }
  }
}

std::size_t tuple_rw_managed::erase(const dynamic_collection_rw &collection) {
  return erase(const_cast<dynamic_collection_rw &>(collection).begin(),
               const_cast<dynamic_collection_rw &>(collection).end());
}

} // namespace fptu
