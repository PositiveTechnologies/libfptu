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

#pragma once
#include "fast_positive/details/uio.h"

#include "fast_positive/details/warnings_push_pt.h"

/* Внутренний тип соответствующий 32-битной ячейке с данными. */
union fptu_unit;

/* Поле кортежа.
 *
 * Фактически это дескриптор поля, в котором записаны: тип данных,
 * номер колонки и смещение к данным. */
struct fptu_field;

/* Представление сериализованной формы кортежа.
 *
 * Фактические это просто системная структура iovec, т.е. указатель
 * на буфер с данными и размер этих данных в байтах. Системный тип struct
 * iovec выбран для совместимости с функциями readv(), writev() и т.п.
 * Другими словами, это просто "оболочка", а сами данные кортежа должны быть
 * где-то размещены. */
union fptu_ro {
  struct {
    const fptu_unit *units;
    size_t total_bytes;
  };
  struct iovec sys;
};

/* Изменяемая форма кортежа.
 * Является плоским буфером, в начале которого расположены служебные поля.
 *
 * Инициализируется функциями fptu_init(), fptu_alloc() и fptu_fetch(). */
struct fptu_rw;

#include "fast_positive/details/warnings_pop.h"
