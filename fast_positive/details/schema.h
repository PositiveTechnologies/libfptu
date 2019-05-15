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
#include "fast_positive/details/api.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/string_view.h"
#include "fast_positive/details/token.h"

#include <memory>
#include <string>
#include <vector>

#include "fast_positive/details/warnings_push_pt.h"

namespace fptu {
class FPTU_API_TYPE schema {
public:
  static std::unique_ptr<schema>
  create() /* фабричный метод для сокрытия подробностей реализации схемы (и
              устранения зависимостей по h-файлам) */
      ;
  virtual ~schema();

  /* 0. Loose и Preplaced поля:
   *     - TODO: TBD.
   *
   * 1. Ограничение на имена полей на уровне справочника схемы:
   *     - Имена полей являются бинарными строками (последовательностями байт),
   *       отсутствуют понятия "регистр символов" и "кодировка", допускаются
   *       любые символы (в том числе непечатные, включая \0);
   *     - Имена всех полей в схеме должны быть уникальными;
   *     - Длина имени должна быть от 0 до 42 байт включительно,
   *       т.е. пустое имя (нулевой длины) является допустимым.
   *
   */

  using token_vector = std::vector<token>;

  /* Оценивает размер кортежа для переданного набора токенов и ожидаемого
   * среднего размера поле переменной длины. */
  static std::size_t
  estimate_tuple_size(const token_vector &,
                      std::size_t expected_average_stretchy_length = 42);

  // Наполнение/определение схемы ----------------------------------------------

  /* Добавляет preplaced-поле встроенного типа.
   * Определяемые pleplaced-поля будут распологаться последовательно в порядке
   * добавления с учетом выравнивания в начале кортежей.
   *
   * Аргумент discernible_null задаёт различимость NULL/NIL от нулевых и пустых
   * значений (строк нулевой длины).
   * Если distinguishable_null задан TRUE, то при попытке чтения отсутствующих
   * значений будет вбрасываться исключение fptu::field_absent, а для
   * preplaced-полей фиксированного размера для обозначения "пусто" будут
   * использоваться предопределенные зарезервированные значения (designated
   * null/empty). Если же distinguishable_null задан FALSE, то при чтении
   * отсутствующих значений полей переменного размера, вместо генерации
   * исключений будет, подставляться естественное нулевое значение
   * соответствующее типу данных, например 0 или пустая строка. Кроме этого,
   * такие пустые значения не будут храниться за ненадобностью. Подробности в
   * описании fptu::token_operations<>::discernible_null().
   *
   * Аргумент saturation определяет поведение при арифметический операциях и
   * присвоении значений. Если saturation равен false, то при переполнении
   * и/или передаче значения вне домена поля будет вброшено одно из исключений
   * fptu::value_denil, fptu::value_range, fptu::value_too_long. Если же
   * saturation равен true, то значение будет автоматически заменено
   * ближайшим допустимым.
   *
   * Аргумент initial_value может задавать опциональное инициализирующее
   * значение, которое будет помещать в новый кортеж при создании и/или сбросе.
   * Если же initial_value равен nullptr, то будут использованы нули.
   * В любом случае, соответствующие данные будут помещены/скопированы внутрь
   * preplaced_image_ и будут там храниться до разрушения экземпляра схемы. */
  virtual token define_preplaced(std::string &&field_name, fptu::genus type,
                                 const bool discernible_null = false,
                                 const bool saturation = false,
                                 const void *initial_value = nullptr) = 0;

  /* Добавляет preplaced-поле с типом "непрозрачная структура".
   * Фактически производится логическое резервирование места для структурного
   * типа, поведение которого полностью контролируется пользователем.
   * Читать/писать такие данные можно будет только как бинарные строки.
   *
   * Аргумент initial_value может задавать опциональное инициализирующее
   * значение, которое будет помещать в новый кортеж при создании и/или сбросе.
   * Если же initial_value равен nullptr, то будут использованы нули.
   * В любом случае, соответствующие данные будут помещены/скопированы внутрь
   * preplaced_image_ и будут там храниться до разрушения экземпляра схемы.
   *
   * Следует обратить внимание, что для добавленных таким образом определений
   * и определений добавляемых через ранее созданные токены намерено допускается
   * перекрытие. Таким образом, становится возможным определять как непрозрачные
   * C-структуры целиком, так и отдельные типизированные поля внутри них. */
  virtual token
  define_preplaced_fixed_opacity(std::string &&name, std::size_t size,
                                 std::size_t align,
                                 const void *initial_value = nullptr) = 0;
  /* Добавляет loose-поле встроенного типа.
   *
   * Аргумент collection определяет может ли поле повторяться и таким образом
   * образовывать итерируемые неупорядоченные коллекции. Если collection = TRUE,
   * то поле может быть добавлено несколько раз посредством метода
   * tuple::insert(), а затем проитерированно посредством tuple::collection().
   *
   * Аргумент discernible_null задаёт различимость NULL/NIL от нулевых и пустых
   * значений (строк нулевой длины).
   * Если distinguishable_null задан TRUE, то при попытке чтения отсутствующих
   * полей будет вбрасываться исключение fptu::field_absent.
   * Если же distinguishable_null задан FALSE, то при чтении отсутствующих полей
   * вместо генерации исключений будет подставляться естественное нулевое
   * значение соответствующее типу данных, например 0 или пустая строка. Кроме
   * этого, поля с такими пустыми значениями не будут храниться за
   * ненадобностью. Подробности вописании
   * fptu::token_operations<>::discernible_null().
   *
   * Аргумент saturation определяет поведение при арифметический операциях и
   * присвоении значений. Если saturation равен false, то при переполнении
   * и/или передаче значения вне домена поля будет вброшено одно из исключений
   * fptu::value_denil, fptu::value_range, fptu::value_too_long. Если же
   * saturation равен true, то значение будет автоматически заменено
   * ближайшим допустимым. */
  virtual token define_loose(std::string &&name, fptu::genus type,
                             const bool collection = false,
                             const bool discernible_null = false,
                             const bool saturation = false) = 0;

  /* Вспомогательный метод добавления поля (не коллекции).
   * В зависимости от аргумента preplaced внутри себя вызывает либо
   * define_preplaced(), либо define_loose(). */
  token define_field(bool preplaced, std::string &&name, fptu::genus type,
                     const bool discernible_null = false,
                     const bool saturation = false);

  /* Вспомогательный метод добавления поля-коллекции.
   * Внутри себя вызывает define_loose() с соответствующми аргументами. */
  token define_collection(std::string &&name, fptu::genus type,
                          const bool discernible_null = false,
                          const bool saturation = false);

  /* Добавляет поле по уже созданному токену. Может использоваться для
   * добавления токенов сгенерированных для полей C-структур, для переноса полей
   * между схемами или для слияния определений схем.
   *
   * Если аргумент renominate задан как true, то будут переопределены
   * идентификатор для loose-поля или расположение для preplaced-поля. Иначе
   * говоря, определение поля будет добавлено без образования зазоров в данных
   * или внутренних идентификаторах, а соответствующий ему новый токен, скорее
   * всего, будет отличаться от переданного в функцию.
   *
   * Следует обратить внимание, что для добавленных таким образом определений
   * и определений непрозрачных preplaced C-структур намерено допускается
   * перекрытие. Таким образом, становится возможным определять как непрозрачные
   * C-структуры целиком, так и отдельные типизированные поля внутри них. */
  virtual token import_definition(std::string &&name, const token &,
                                  const void *initial_value = nullptr,
                                  const bool renominate = false) = 0;

  // Получеие токенов по именам полей ------------------------------------------
  enum class boolean_option {
    option_default,
    option_enforce_false,
    option_enforce_true
  };
  virtual token get_token_nothrow(
      const string_view &field_name,
      const boolean_option discernible_null = boolean_option::option_default,
      const boolean_option saturated = boolean_option::option_default) const
      noexcept = 0;

  token get_token(
      const string_view &field_name,
      const boolean_option discernible_null = boolean_option::option_default,
      const boolean_option saturated = boolean_option::option_default) const;

  token operator[](const string_view &field_name) const {
    return get_token(field_name);
  }

  token operator[](const std::string &field_name) const {
    return get_token(string_view(field_name));
  }

  virtual token get_token_nothrow(
      const token &inlay_token, const string_view &inner_name,
      const boolean_option discernible_null = boolean_option::option_default,
      const boolean_option saturated = boolean_option::option_default) const
      noexcept = 0;
  token get_token(
      const token &inlay_token, const string_view &inner_name,
      const boolean_option discernible_null = boolean_option::option_default,
      const boolean_option saturated = boolean_option::option_default) const;

  // Получение имён полей по токенам -------------------------------------------
  virtual string_view get_name_nothrow(const token &) const noexcept = 0;
  string_view get_name(const token &ident) const;
  string_view operator[](const token &token) const { return get_name(token); }

  // Собственные потребности libfptu -------------------------------------------
  const token_vector &tokens() const { return sorted_tokens_; }
  std::size_t preplaced_bytes() const noexcept {
    return preplaced_image_.size();
  }
  std::size_t preplaced_units() const noexcept {
    return details::bytes2units(preplaced_bytes());
  }
  const void *preplaced_init_image() const { return preplaced_image_.data(); }
  std::size_t stretchy_preplaced() const { return stretchy_preplaced_; }

protected:
  inline schema();
  std::size_t stretchy_preplaced_;
  std::string preplaced_image_;
  token_vector sorted_tokens_;
};

} // namespace fptu

#include "fast_positive/details/warnings_pop.h"
