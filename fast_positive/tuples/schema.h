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
#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/essentials.h"
#include "fast_positive/tuples/string_view.h"
#include "fast_positive/tuples/token.h"

#include <memory>
#include <string>
#include <vector>

#include "fast_positive/tuples/details/warnings_push_pt.h"

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
  __pure_function static std::size_t
  estimate_tuple_size(const token_vector &,
                      std::size_t expected_average_stretchy_length = 42);

  // Наполнение/определение схемы ----------------------------------------------

  /* Добавляет preplaced-поле встроенного типа.
   * Определяемые pleplaced-поля будут распологаться последовательно в порядке
   * добавления с учетом выравнивания в начале кортежей.
   *
   * Аргумент discernible_null задаёт различимость NULL/NIL от нулевых и пустых
   * значений (строк нулевой длины).
   * Если discernible_null задан TRUE, то при попытке чтения отсутствующих
   * значений будет вбрасываться исключение fptu::field_absent, а для
   * preplaced-полей фиксированного размера для обозначения "пусто" будут
   * использоваться предопределенные зарезервированные значения (designated
   * null/empty). Если же discernible_null задан FALSE, то при чтении
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
   * Если discernible_null задан TRUE, то при попытке чтения отсутствующих
   * полей будет вбрасываться исключение fptu::field_absent.
   * Если же discernible_null задан FALSE, то при чтении отсутствующих полей
   * вместо генерации исключений будет подставляться естественное нулевое
   * значение соответствующее типу данных, например 0 или пустая строка. Кроме
   * этого, поля с такими пустыми значениями не будут храниться за
   * ненадобностью. Подробности вописании
   * fptu::token_operations<>::is_discernible_null().
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
  __pure_function virtual token get_token_nothrow(
      const string_view &field_name,
      const boolean_option discernible_null = boolean_option::option_default,
      const boolean_option saturated = boolean_option::option_default) const
      noexcept = 0;

  __pure_function token get_token(
      const string_view &field_name,
      const boolean_option discernible_null = boolean_option::option_default,
      const boolean_option saturated = boolean_option::option_default) const;

  token operator[](const string_view &field_name) const {
    return get_token(field_name);
  }

  token operator[](const std::string &field_name) const {
    return get_token(string_view(field_name));
  }

  __pure_function virtual token get_token_nothrow(
      const token &inlay_token, const string_view &inner_name,
      const boolean_option discernible_null = boolean_option::option_default,
      const boolean_option saturated = boolean_option::option_default) const
      noexcept = 0;
  __pure_function token get_token(
      const token &inlay_token, const string_view &inner_name,
      const boolean_option discernible_null = boolean_option::option_default,
      const boolean_option saturated = boolean_option::option_default) const;

  // Получение имён полей по токенам -------------------------------------------
  __pure_function virtual string_view get_name_nothrow(const token &) const
      noexcept = 0;
  __pure_function string_view get_name(const token &ident) const;
  string_view operator[](const token &token) const { return get_name(token); }

  // Собственные потребности libfptu -------------------------------------------
  cxx14_constexpr const token_vector &tokens() const noexcept {
    return sorted_tokens_;
  }
  std::size_t preplaced_bytes() const noexcept {
    return preplaced_image_.size();
  }
  std::size_t preplaced_units() const noexcept {
    return details::bytes2units(preplaced_bytes());
  }
  const void *preplaced_init_image() const noexcept {
    return preplaced_image_.data();
  }
  cxx14_constexpr std::size_t number_of_preplaced() const noexcept {
    return number_of_preplaced_;
  }
  cxx14_constexpr bool have_preplaced() const noexcept {
    return number_of_preplaced() > 0;
  }
  cxx14_constexpr std::size_t number_of_stretchy_preplaced() const noexcept {
    return number_of_stretchy_preplaced_;
  }
  cxx14_constexpr bool have_stretchy_preplaced() const noexcept {
    return number_of_stretchy_preplaced() > 0;
  }

  __pure_function token by_loose(const details::field_loose *) const noexcept;
  __pure_function token by_offset(const ptrdiff_t offset) const noexcept;
  __pure_function token next_by_offset(const ptrdiff_t offset) const noexcept;
  __pure_function token prev_by_offset(const ptrdiff_t offset) const noexcept;

protected:
  inline schema();
  unsigned number_of_preplaced_, number_of_stretchy_preplaced_;
  std::string preplaced_image_;
  token_vector sorted_tokens_;

  token_vector::const_iterator search_preplaced(const ptrdiff_t offset) const
      noexcept;
};

} // namespace fptu

#include "fast_positive/tuples/details/warnings_pop.h"
