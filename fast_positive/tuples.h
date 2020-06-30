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

/*
 * libfptu = { Fast Positive Tuples, aka Позитивные Кортежи }
 * Машинно-эффективный формат линейного представления небольших структур данных
 * для (де)сериализации, обмена сообщениями и размещения в разделяемой памяти.
 *
 * Всё будет хорошо. The Future will (be) Positive
 * https://github.com/PositiveTechnologies/libfptu
 *
 * Machine-handy format for linear representation of small data structures
 * for (de)serialization, messaging and placement in shared memory.
 */

#pragma once

//------------------------------------------------------------------------------
#include "fast_positive/tuples/details/warnings_push_pt.h"

#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/types.h"

#ifdef __cplusplus

#include "fast_positive/tuples/details/warnings_push_system.h"

#include <cmath>     // for std::ldexp
#include <limits>    // for numeric_limits<>
#include <memory>    // for std::uniq_ptr
#include <ostream>   // for std::ostream
#include <stdexcept> // for std::invalid_argument
#include <string>    // for std::string

#include "fast_positive/tuples/details/warnings_pop.h"

#include "fast_positive/tuples/details/audit.h"
#include "fast_positive/tuples/details/exceptions.h"
#include "fast_positive/tuples/details/field.h"
#include "fast_positive/tuples/details/getter.h"
#include "fast_positive/tuples/details/legacy_common.h"
#include "fast_positive/tuples/details/ro.h"
#include "fast_positive/tuples/details/rw.h"
#include "fast_positive/tuples/details/tagged_pointer.h"
#include "fast_positive/tuples/essentials.h"
#include "fast_positive/tuples/schema.h"
#include "fast_positive/tuples/token.h"

#endif /* __cplusplus */

//------------------------------------------------------------------------------

namespace fptu {

class schema /* Минималистический справочник схемы.
   Содержит необходимую для машины информацию об именах полей, их
   внутренних атрибутах и типе хранимых значений.

   С точки зрения API, главное назначение справочника схемы:
   трансляция символических ("человеческих") имен полей в "токены
   доступа".

   Полное определение в fast_positive/tuples/schema.h */
    ;

/* Режимы проверки данных и их соответствия схеме при создании кортежей */
enum validation_mode {
  default_validation,
  enforce_skip_validation,
  enforce_validation
};
cxx14_constexpr bool apply_validation_mode(validation_mode mode,
                                           bool by_defaults) cxx11_noexcept {
  switch (mode) {
  default:
    return by_defaults;
  case enforce_skip_validation:
    return false;
  case enforce_validation:
    return true;
  }
}

cxx11_constexpr validation_mode
combine_validation_mode(validation_mode mode, bool by_default) cxx11_noexcept {
  return (mode != default_validation)
             ? mode
             : by_default ? enforce_validation : enforce_skip_validation;
}

class token /* Токен доступа к полю кортежей.
  Является именем поля кортежа оттранслированным в координаты машинного
  представления схемы. В компактном и удобном для машины виде содержит всю
  необходимую информацию:
    - тип данных поля;
    - вид поля быстрое/медленное/коллекция или поле вложенной структуры;
    - смещение к данным для "быстрых" полей.
    и т.д.

  Полное определение в fast_positive/tuples/token.h */
    ;

class variant_value /* Класс поддержки динамической типизации.
  Похож на boost::variant и fpta_value. Принципиальное отличие в том, что класс
  не хранит значение внутри, а является обёрнутым тэгированным указателем (тип
  данных хранится в тэге).

  Соответственно, класс единообразно предоставляет доступ к значениями внутри
  кортежей, значениям нативных переменных C++, строкам std::string, константным
  строкам в «пуле строк» и т.д. */
{
  using pimpl = tagged_pointer<void, wide_tagged_pointer_base>;

  /* 3 бита */
  enum kind_pointer : unsigned {
    none = 0 /* nullptr или обычный не-тегированный указатель */,
    pod_value = 1 /* указатель на: значение POD-типа (C-string или простой тип
                     фиксированного размера) */
    ,
    extobj_value = 2 /* указатель на: внешне управляемый объект или переменную
                  внешнего типа (в том числе "список") */
    ,
    long_managed = 3 /* указатель на: значение типа переменного размера (в
                        управляемом буфере) */
    ,
    varlen_pool = 4 /* указатель на: значение типа переменного размера в пуле
                       констант */
    ,
    field_ro =
        5 /* указатель на: данные поля в RO-кортеже с типом данных в теге */
    ,
    field_rw =
        6 /* указатель на: данные поля в RW-кортеже с типом данных в теге */
    ,
    field_rw_incorporeal =
        7 /* указатель на: RW-кортеж с id поля внутри (бестелесное
       поле, которое при присвоении превратиться в field_rw) */
  };

  /* TBD / TODO */
};

/* Линейка масштабов для удобного создания кортежей */
enum initiation_scale {
  tiny /* 1/256 ≈1K:32 */,
  small /* 1/64 ≈4K:128 */,
  medium /* 1/16 ≈16K:512 */,
  large /* 1/4 ≈64K:2028 */,
  extreme /* максимальный ≈256K:8192 */,

  scale_default = tiny
};
std::size_t estimate_space_for_tuple(const initiation_scale &scale,
                                     const schema *schema);

/* Параметры по-умолчанию.
 * Пустотелая структура используется как теговый тип при вызове конструкторов.
 * Актуальные значения хранятся в статических членах внутри FPTU и
 * устанавливаются статическим методом. */
struct FPTU_API_TYPE defaults {
  static std::unique_ptr<fptu::schema> schema;
  static hippeus::buffer_tag allot_tag;
  static initiation_scale scale;
  static void
  setup(const initiation_scale &scale, std::unique_ptr<fptu::schema> &&schema,
        const hippeus::buffer_tag &allot_tag = fptu::defaults::allot_tag);
  static std::size_t
  estimate_space_for_tuple(const initiation_scale &_scale = scale) {
    return fptu::estimate_space_for_tuple(_scale, schema.get());
  }
};

//------------------------------------------------------------------------------

/* forward-декларация основных классов для взаимодействия между ними */
class tuple_ro_weak;
class tuple_ro_managed;
class tuple_rw_fixed;
class tuple_rw_managed;

/* аллокатор управляемых буферов по-умолчанию */
static inline hippeus::buffer_tag default_buffer_allot() cxx11_noexcept {
  return defaults::allot_tag;
}

template <typename TUPLE>
class tuple_crtp_reader
/* CRTP шаблон для вытягивания функционала из PIMPL-указателя.
   https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
   https://en.cppreference.com/w/cpp/language/pimpl

   Обеспечивает получение значений полей посредством функций get_TYPE(token),
   а также оператор индексирования operator[token], который возвращает
   промежуточный объект доступа с набором собственных геттеров.

   Доступ к коллекциям (повторяющимся полям) предоставляется через
   промежуточный range-объект возвращаемый функцией collection(), примерно так:
     for(auto field: tuple.collection(token))
       sum += field.get_integer();

   ВОПРОСЫ ДИЗАЙНА:
   CRTP-обёртка здесь преследует три НЕ-принципиальные цели:
     - спрятать различные PIMPL-указатели, т.е. избавиться от синтаксиса
       использования указателя при чтении кортежа.
     - концентрированно представить формальное описание интерфейса,
       проще говоря чтобы видеть все функциональные методы в одном месте.
     - как еще один слой абстракции/изоляции, в котором можно что-по
       подкрутить без изменения ABI библиотеки. В том числе, теоретически
       можно вообще подменить реализацию.

   Тем не менее, CRTP НЕ является неотъемлемой или шибко нужно частью,
   от него можно избавиться.

   В свою очередь, PIMPL или "просто указатель" нужен так как кортеж в читаемой
   сериализованной форме не имеет конструктора (это просто участок памяти).

   PIMPL здесь не приводит к увеличению накладных расходов, так как ABI
   по-прежнему работает с одним указателем на плоские читаемые данные кортежа.
   При этом PIMPL-обёртку следует передавать по-значению, т.е. достаточно
   НЕ добавлять еще один уровень косвенности. */
{
private:
  cxx11_constexpr const TUPLE &self() const cxx11_noexcept {
    return *static_cast<const TUPLE *>(this);
  }

public:
  __pure_function cxx11_constexpr const details::tuple_ro *
  get_impl() const cxx11_noexcept {
    return self().pimpl_;
  }

#define HERE_CRTP_MAKE(RETURN_TYPE, NAME)                                      \
  template <typename TOKEN>                                                    \
  __pure_function inline RETURN_TYPE NAME(const TOKEN &ident) const {          \
    return get_impl()->NAME(ident);                                            \
  }
  HERE_CRTP_MAKE(details::accessor_ro<TOKEN>, operator[])
  HERE_CRTP_MAKE(details::collection_ro<TOKEN>, collection)
  HERE_CRTP_MAKE(bool, is_present)
  HERE_CRTP_MAKE(string_view, get_string)
  HERE_CRTP_MAKE(string_view, get_varbinary)
  HERE_CRTP_MAKE(property_pair, get_property)
  HERE_CRTP_MAKE(bool, get_bool)
  HERE_CRTP_MAKE(short, get_enum)
  HERE_CRTP_MAKE(int8_t, get_i8)
  HERE_CRTP_MAKE(uint8_t, get_u8)
  HERE_CRTP_MAKE(int16_t, get_i16)
  HERE_CRTP_MAKE(uint16_t, get_u16)
  HERE_CRTP_MAKE(int32_t, get_i32)
  HERE_CRTP_MAKE(uint32_t, get_u32)
  HERE_CRTP_MAKE(int64_t, get_i64)
  HERE_CRTP_MAKE(uint64_t, get_u64)
  HERE_CRTP_MAKE(float, get_f32)
  HERE_CRTP_MAKE(double, get_f64)
  HERE_CRTP_MAKE(decimal64, get_decimal)
  HERE_CRTP_MAKE(datetime_t, get_datetime)
  HERE_CRTP_MAKE(const uuid_t &, get_uuid)
  HERE_CRTP_MAKE(const int128_t &, get_int128)
  HERE_CRTP_MAKE(const uint128_t &, get_uint128)
  HERE_CRTP_MAKE(const binary96_t &, get_bin96)
  HERE_CRTP_MAKE(const binary128_t &, get_bin128)
  HERE_CRTP_MAKE(const binary160_t &, get_bin160)
  HERE_CRTP_MAKE(const binary192_t &, get_bin192)
  HERE_CRTP_MAKE(const binary224_t &, get_bin224)
  HERE_CRTP_MAKE(const binary256_t &, get_bin256)
  HERE_CRTP_MAKE(const binary320_t &, get_bin320)
  HERE_CRTP_MAKE(const binary384_t &, get_bin384)
  HERE_CRTP_MAKE(const binary512_t &, get_bin512)
  HERE_CRTP_MAKE(const ip_address_t &, get_ip_address)
  HERE_CRTP_MAKE(mac_address_t, get_mac_address)
  HERE_CRTP_MAKE(const ip_net_t &, get_ip_net)
  HERE_CRTP_MAKE(int64_t, get_integer)
  HERE_CRTP_MAKE(uint64_t, get_unsigned)
  HERE_CRTP_MAKE(double, get_float)
  HERE_CRTP_MAKE(double, get_number_as_ieee754double)
#undef HERE_CRTP_MAKE

  __pure_function explicit cxx11_constexpr
  operator bool() const cxx11_noexcept {
    return get_impl() != 0;
  }
  __pure_function const char *
  validate(const fptu::schema *schema,
           bool holes_are_not_allowed = false) const cxx11_noexcept {
    return get_impl()->audit(schema, holes_are_not_allowed);
  }
  __pure_function static const char *
  validate(const void *ptr, std::size_t bytes, const fptu::schema *schema,
           bool holes_are_not_allowed = false) cxx11_noexcept {
    return TUPLE::impl::audit(ptr, bytes, schema, holes_are_not_allowed);
  }
  __pure_function cxx11_constexpr bool empty() const cxx11_noexcept {
    return get_impl()->empty();
  }
  __pure_function cxx11_constexpr std::size_t size() const cxx11_noexcept {
    return get_impl()->size();
  }
  __pure_function cxx11_constexpr const void *data() const cxx11_noexcept {
    return get_impl()->data();
  }
  __pure_function cxx14_constexpr std::size_t
  payload_size() const cxx11_noexcept {
    return get_impl()->payload_size();
  }
  __pure_function cxx11_constexpr const void *payload() const cxx11_noexcept {
    return get_impl()->payload();
  }
  __pure_function cxx11_constexpr std::size_t
  index_size() const cxx11_noexcept {
    return get_impl()->index_size();
  }
  __pure_function cxx11_constexpr bool is_sorted() const cxx11_noexcept {
    return get_impl()->is_sorted();
  }
  __pure_function cxx11_constexpr bool have_preplaced() const cxx11_noexcept {
    return get_impl()->have_preplaced();
  }
  __pure_function operator iovec() const cxx11_noexcept {
    return iovec(data(), size());
  }

  //----------------------------------------------------------------------------

  __pure_function field_iterator_ro cbegin() const cxx11_noexcept {
    return get_impl()->cbegin(defaults::schema.get());
  }
  __pure_function field_iterator_ro
  cbegin(const fptu::schema *schema) const cxx11_noexcept {
    return get_impl()->cbegin(schema);
  }
  cxx11_constexpr field_iterator_ro cend() const cxx11_noexcept {
    return get_impl()->cend(defaults::schema.get());
  }
  __pure_function field_iterator_ro
  cend(const fptu::schema *schema) const cxx11_noexcept {
    return get_impl()->cend(schema);
  }

  __pure_function field_iterator_ro begin() const cxx11_noexcept {
    return cbegin();
  }
  cxx11_constexpr field_iterator_ro end() const cxx11_noexcept {
    return cend();
  }
  __pure_function field_iterator_ro
  begin(const fptu::schema *schema) const cxx11_noexcept {
    return cbegin(schema);
  }
  cxx11_constexpr field_iterator_ro
  end(const fptu::schema *schema) const cxx11_noexcept {
    return cend(schema);
  }

  //----------------------------------------------------------------------------

  cxx11_constexpr loose_iterator_ro cbegin_loose() const cxx11_noexcept {
    return get_impl()->cbegin_loose();
  }
  cxx11_constexpr loose_iterator_ro cend_loose() const cxx11_noexcept {
    return get_impl()->cend_loose();
  }

  cxx11_constexpr loose_iterator_ro begin_loose() const cxx11_noexcept {
    return cbegin_loose();
  }
  cxx11_constexpr loose_iterator_ro end_loose() const cxx11_noexcept {
    return cend_loose();
  }
};

class FPTU_API_TYPE tuple_ro_weak : public tuple_crtp_reader<tuple_ro_weak>
/* Не-управляемый R/O-кортеж с данными в отдельном (внешнем) буфере.
  Фактически это просто указатель, поэтому кортеж
  валиден только пока доступны и не изменены данные во внешнем буфере.
  Может быть создан из кортежа любого класса (см далее). */
{
  friend class tuple_crtp_reader<tuple_ro_weak>;
  friend class tuple_ro_managed;
  friend class preplaced_nested;
  template <typename> friend class tuple_crtp_writer;

protected:
  using impl = details::tuple_ro;
  const impl *pimpl_;
  explicit cxx11_constexpr tuple_ro_weak(const impl *ptr) cxx11_noexcept
      : pimpl_(ptr) {}

public:
  cxx11_constexpr tuple_ro_weak() cxx11_noexcept : tuple_ro_weak(nullptr) {}
  tuple_ro_weak(const void *ptr, std::size_t bytes, const fptu::schema *schema,
                validation_mode validation = default_validation)
      : pimpl_(impl::make_from_buffer(
            ptr, bytes, schema, !apply_validation_mode(validation, true))) {}
  ~tuple_ro_weak() =
      /* to tuple_ro_weak was a literal type */
      default /* { pimpl_ = nullptr; } */;

  inline tuple_ro_weak(const tuple_ro_managed &);
  inline tuple_ro_weak(const tuple_rw_fixed &);
  inline tuple_ro_weak(const tuple_rw_managed &);

  cxx11_constexpr tuple_ro_weak(const tuple_ro_weak &) cxx11_noexcept = default;
  tuple_ro_weak &operator=(const tuple_ro_weak &) cxx11_noexcept = default;

  cxx11_constexpr tuple_ro_weak(tuple_ro_weak &&src) cxx11_noexcept = default;
  cxx14_constexpr tuple_ro_weak &operator=(tuple_ro_weak &&src) cxx11_noexcept {
    pimpl_ = src.pimpl_;
    src.pimpl_ = nullptr;
    return *this;
  }
  void swap(tuple_ro_weak &ditto) cxx11_noexcept {
    std::swap(pimpl_, ditto.pimpl_);
  }

  template <typename TOKEN>
  __pure_function inline tuple_ro_weak
  get_nested_weak(const TOKEN &ident) const {
    return tuple_ro_weak(get_impl()->get_nested(ident));
  }

  cxx11_constexpr bool
  operator==(const tuple_ro_weak &ditto) const cxx11_noexcept {
    return pimpl_ == ditto.pimpl_;
  }
  cxx11_constexpr bool
  operator!=(const tuple_ro_weak &ditto) const cxx11_noexcept {
    return pimpl_ != ditto.pimpl_;
  }
  inline bool operator==(const tuple_ro_managed &ditto) const cxx11_noexcept;
  inline bool operator!=(const tuple_ro_managed &ditto) const cxx11_noexcept;
  inline bool operator==(const tuple_rw_fixed &ditto) const cxx11_noexcept;
  inline bool operator!=(const tuple_rw_fixed &ditto) const cxx11_noexcept;
};

class FPTU_API_TYPE tuple_ro_managed
    : public tuple_crtp_reader<tuple_ro_managed>
/* Управляемый R/O-кортеж с данными в управляемом буфере со счётчиком ссылок.
  Поддерживает std::move(), clone(). Может быть создан из tuple_rw_managed
  без копирования данных, а из кортежей других классов с выделением буфера
  и полным копированием данных.

  Во избежание непреднамеренного копирования данных вместо различных копирующих
  конструкторов предоставлено несколько вариантов статического метода clone().

  Если же способ создания предполагает копирование данных в собственный
  управляемый буфер, то при этом создается R/W-форма кортежа. В результате,
  созданный посредством копирования R/O-кортеж может быть преобразован в
  R/W-форму перемещающим конструктором. По этой причине предложено несколько
  клонирующих методов с дополнительными параметрами. */
{
  friend class tuple_crtp_reader<tuple_ro_managed>;
  template <typename TUPLE> friend class tuple_crtp_writer;
  friend class tuple_rw_fixed;

protected:
  using impl = details::tuple_ro;
  const impl *pimpl_;
  const hippeus::buffer *hb_;

  __cold __noreturn void throw_buffer_mismatch() const;
  inline void check_buffer() const;
  explicit tuple_ro_managed(
      const impl *ro,
      const hippeus::buffer *buffer /* увеличивает счетчик ссылок */);

  /* Клонирующие копирующие конструкторы - спрятаны с предоставлением взамен
   * статических методов clone(), чтобы не допускать неявного клонирования
   * данных. */
  tuple_ro_managed(const tuple_rw_fixed &src,
                   const hippeus::buffer_tag &allot_tag);
  /* Возвращает указатель на опорную R/W-форму, если таковая была скрыто
   * порождена при создании экземпляра tuple_ro_managed посредством копирования
   * данных в собственный буфер. */
  __pure_function const details::tuple_rw *peek_basis() const cxx11_noexcept;

public:
  const hippeus::buffer *get_buffer() const cxx11_noexcept { return hb_; }
  tuple_ro_managed() cxx11_noexcept : pimpl_(nullptr), hb_(nullptr) {}
  void purge() {
    hb_->detach(/* accepts nullptr */);
    pimpl_ = nullptr;
    hb_ = nullptr;
  }
  ~tuple_ro_managed() { purge(); }

  /* Конструктор из внешнего источника с выделением управляемого буфера
   * и копированием данных.
   * (!) Если создаваемый экземпляр tuple_ro_managed впоследствии предполагается
   * конвертировать в R/W-форму, то его следует создавать из tuple_rw_fixed
   * примерно так:
   *    tuple_ro_managed(tuple_rw_fixed(...)) */
  tuple_ro_managed(
      const void *source_ptr, std::size_t source_bytes,
      const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  /* Клонирование из уже проверенной R/O-формы в собственный буфер.
   * (!) Если создаваемый экземпляр tuple_ro_managed впоследствии предполагается
   * конвертировать в R/W-форму, то его следует создавать из tuple_rw_fixed
   * примерно так:
   *    tuple_ro_managed(tuple_rw_fixed(...)) */
  static tuple_ro_managed
  clone(const tuple_ro_weak &src, const fptu::schema *schema = nullptr,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot(),
        validation_mode validation = default_validation);

  /* Возвращает указатель на схему из опорной R/W-формы, если таковая была
   * скрыто порождена при создании экземпляра tuple_ro_managed посредством
   * копирования данных в собственный буфер, и при этом была задана схема. */
  __pure_function inline const fptu::schema *peek_schema() const cxx11_noexcept;

  static tuple_ro_managed
  clone(const tuple_ro_managed &src, const fptu::schema *schema = nullptr,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  static tuple_ro_managed
  clone(const tuple_rw_fixed &src,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return tuple_ro_managed(src, allot_tag);
  }

  explicit tuple_ro_managed(const tuple_ro_weak &nested_weak,
                            const tuple_ro_managed &managed_master)
      : tuple_ro_managed(nested_weak ? nested_weak.get_impl() : nullptr,
                         nested_weak ? managed_master.get_buffer() : nullptr) {}

  tuple_ro_managed(const tuple_ro_managed &src)
      : tuple_ro_managed(src.pimpl_, src.hb_) {}

  tuple_ro_managed &operator=(const tuple_ro_managed &src);

  tuple_ro_managed(tuple_rw_fixed &&src);

  tuple_ro_managed &operator=(tuple_rw_fixed &&src);

  tuple_ro_managed(tuple_ro_managed &&src);

  tuple_ro_managed &operator=(tuple_ro_managed &&src);

  void swap(tuple_ro_managed &ditto) cxx11_noexcept {
    std::swap(pimpl_, ditto.pimpl_);
    std::swap(hb_, ditto.hb_);
  }

  tuple_ro_weak take_weak() const cxx11_noexcept {
    return tuple_ro_weak(pimpl_);
  }

  bool operator==(const tuple_ro_weak &ditto) const cxx11_noexcept {
    return pimpl_ == ditto.pimpl_;
  }
  bool operator!=(const tuple_ro_weak &ditto) const cxx11_noexcept {
    return pimpl_ != ditto.pimpl_;
  }
  bool operator==(const tuple_ro_managed &ditto) const cxx11_noexcept {
    return pimpl_ == ditto.pimpl_;
  }
  bool operator!=(const tuple_ro_managed &ditto) const cxx11_noexcept {
    return pimpl_ != ditto.pimpl_;
  }
  inline bool operator==(const tuple_rw_fixed &ditto) const cxx11_noexcept;
  inline bool operator!=(const tuple_rw_fixed &ditto) const cxx11_noexcept;

  template <typename TOKEN>
  __pure_function inline tuple_ro_weak
  get_nested_weak(const TOKEN &ident) const {
    return tuple_ro_weak(get_impl()->get_nested(ident));
  }

  template <typename TOKEN>
  __pure_function inline tuple_ro_managed
  get_nested_managed(const TOKEN &ident) const {
    return tuple_ro_managed(get_impl()->get_nested(ident), get_buffer());
  }
};

//------------------------------------------------------------------------------

template <typename TUPLE> class tuple_crtp_writer {
private:
  cxx14_constexpr const TUPLE &self() const cxx11_noexcept {
    return *static_cast<const TUPLE *>(this);
  }
  cxx14_constexpr TUPLE &self() cxx11_noexcept {
    return *static_cast<TUPLE *>(this);
  }

protected:
  cxx14_constexpr details::tuple_rw *get_impl() cxx11_noexcept {
    return self().pimpl_;
  }

public:
  cxx14_constexpr const details::tuple_rw *get_impl() const cxx11_noexcept {
    return self().pimpl_;
  }

#define HERE_CRTP_MAKE(RETURN_TYPE, NAME)                                      \
  template <typename TOKEN>                                                    \
  __pure_function inline RETURN_TYPE NAME(const TOKEN &ident) const {          \
    return get_impl()->NAME(ident);                                            \
  }
  HERE_CRTP_MAKE(details::accessor_ro<TOKEN>, operator[])
  HERE_CRTP_MAKE(details::collection_ro<TOKEN>, collection)
  HERE_CRTP_MAKE(bool, is_present)
  HERE_CRTP_MAKE(string_view, get_string)
  HERE_CRTP_MAKE(string_view, get_varbinary)
  HERE_CRTP_MAKE(property_pair, get_property)
  HERE_CRTP_MAKE(bool, get_bool)
  HERE_CRTP_MAKE(short, get_enum)
  HERE_CRTP_MAKE(int8_t, get_i8)
  HERE_CRTP_MAKE(uint8_t, get_u8)
  HERE_CRTP_MAKE(int16_t, get_i16)
  HERE_CRTP_MAKE(uint16_t, get_u16)
  HERE_CRTP_MAKE(int32_t, get_i32)
  HERE_CRTP_MAKE(uint32_t, get_u32)
  HERE_CRTP_MAKE(int64_t, get_i64)
  HERE_CRTP_MAKE(uint64_t, get_u64)
  HERE_CRTP_MAKE(float, get_f32)
  HERE_CRTP_MAKE(double, get_f64)
  HERE_CRTP_MAKE(decimal64, get_decimal)
  HERE_CRTP_MAKE(datetime_t, get_datetime)
  HERE_CRTP_MAKE(const uuid_t &, get_uuid)
  HERE_CRTP_MAKE(const int128_t &, get_int128)
  HERE_CRTP_MAKE(const uint128_t &, get_uint128)
  HERE_CRTP_MAKE(const binary96_t &, get_bin96)
  HERE_CRTP_MAKE(const binary128_t &, get_bin128)
  HERE_CRTP_MAKE(const binary160_t &, get_bin160)
  HERE_CRTP_MAKE(const binary192_t &, get_bin192)
  HERE_CRTP_MAKE(const binary224_t &, get_bin224)
  HERE_CRTP_MAKE(const binary256_t &, get_bin256)
  HERE_CRTP_MAKE(const binary320_t &, get_bin320)
  HERE_CRTP_MAKE(const binary384_t &, get_bin384)
  HERE_CRTP_MAKE(const binary512_t &, get_bin512)
  HERE_CRTP_MAKE(const ip_address_t &, get_ip_address)
  HERE_CRTP_MAKE(mac_address_t, get_mac_address)
  HERE_CRTP_MAKE(const ip_net_t &, get_ip_net)
  HERE_CRTP_MAKE(int64_t, get_integer)
  HERE_CRTP_MAKE(uint64_t, get_unsigned)
  HERE_CRTP_MAKE(double, get_float)
  HERE_CRTP_MAKE(double, get_number_as_ieee754double)
#undef HERE_CRTP_MAKE

  template <typename TOKEN>
  __pure_function inline tuple_ro_weak
  get_nested_weak(const TOKEN &ident) const {
    return tuple_ro_weak(get_impl()->get_nested(ident));
  }

  template <typename TOKEN>
  __pure_function inline tuple_ro_managed
  get_nested_managed(const TOKEN &ident) const {
    return tuple_ro_managed(get_impl()->get_nested(ident), get_buffer());
  }

#define HERE_CRTP_MAKE(VALUE_TYPE, NAME)                                       \
  template <typename TOKEN>                                                    \
  inline void set_##NAME(const TOKEN &ident, const VALUE_TYPE value) {         \
    get_impl()->set_##NAME(ident, value);                                      \
  }                                                                            \
  inline details::tuple_rw::collection_iterator_rw<token> insert_##NAME(       \
      const token &ident, const VALUE_TYPE value) {                            \
    return get_impl()->insert_##NAME(ident, value);                            \
  }
  HERE_CRTP_MAKE(string_view &, string)
  HERE_CRTP_MAKE(string_view &, varbinary)
  HERE_CRTP_MAKE(property_pair &, property)
  HERE_CRTP_MAKE(bool, bool)
  HERE_CRTP_MAKE(short, enum)
  HERE_CRTP_MAKE(int8_t, i8)
  HERE_CRTP_MAKE(uint8_t, u8)
  HERE_CRTP_MAKE(int16_t, i16)
  HERE_CRTP_MAKE(uint16_t, u16)
  HERE_CRTP_MAKE(int32_t, i32)
  HERE_CRTP_MAKE(uint32_t, u32)
  HERE_CRTP_MAKE(int64_t, i64)
  HERE_CRTP_MAKE(uint64_t, u64)
  HERE_CRTP_MAKE(float, f32)
  HERE_CRTP_MAKE(double, f64)
  HERE_CRTP_MAKE(decimal64, decimal)
  HERE_CRTP_MAKE(datetime_t, datetime)
  HERE_CRTP_MAKE(uuid_t &, uuid)
  HERE_CRTP_MAKE(int128_t &, int128)
  HERE_CRTP_MAKE(uint128_t &, uint128)
  HERE_CRTP_MAKE(binary96_t &, bin96)
  HERE_CRTP_MAKE(binary128_t &, bin128)
  HERE_CRTP_MAKE(binary160_t &, bin160)
  HERE_CRTP_MAKE(binary192_t &, bin192)
  HERE_CRTP_MAKE(binary224_t &, bin224)
  HERE_CRTP_MAKE(binary256_t &, bin256)
  HERE_CRTP_MAKE(binary320_t &, bin320)
  HERE_CRTP_MAKE(binary384_t &, bin384)
  HERE_CRTP_MAKE(binary512_t &, bin512)
  HERE_CRTP_MAKE(ip_address_t &, ip_address)
  HERE_CRTP_MAKE(mac_address_t, mac_address)
  HERE_CRTP_MAKE(ip_net_t &, ip_net)
  HERE_CRTP_MAKE(int64_t, integer)
  HERE_CRTP_MAKE(uint64_t, integer)
  HERE_CRTP_MAKE(int64_t, unsigned)
  HERE_CRTP_MAKE(uint64_t, unsigned)
  HERE_CRTP_MAKE(double, float)
  HERE_CRTP_MAKE(int32_t, number)
  HERE_CRTP_MAKE(uint32_t, number)
  HERE_CRTP_MAKE(int64_t, number)
  HERE_CRTP_MAKE(uint64_t, number)
  HERE_CRTP_MAKE(float, number)
  HERE_CRTP_MAKE(double, number)
#undef HERE_CRTP_MAKE

  template <typename TOKEN>
  inline void set_nested(const TOKEN &ident, const tuple_ro_weak &value) {
    get_impl()->set_nested(ident, value.get_impl());
  }
  details::tuple_rw::collection_iterator_rw<token>
  insert_nested(const token &ident, const tuple_ro_weak &value) {
    return get_impl()->insert_nested(ident, value.get_impl());
  }

  template <typename TOKEN>
  inline void set_string(const TOKEN &ident, const std::string &value) {
    get_impl()->set_string(ident, fptu::string_view(value));
  }
  details::tuple_rw::collection_iterator_rw<token>
  insert_string(const token &ident, const std::string &value) {
    return get_impl()->insert_string(ident, fptu::string_view(value));
  }

  __pure_function cxx11_constexpr const hippeus::buffer *
  get_buffer() const cxx11_noexcept {
    return get_impl()->get_buffer();
  }
  __pure_function cxx11_constexpr const fptu::schema *
  schema() const cxx11_noexcept {
    return get_impl()->schema();
  }

  __pure_function explicit cxx11_constexpr
  operator bool() const cxx11_noexcept {
    return get_impl() != 0;
  }
  __pure_function const char *validate() const cxx11_noexcept {
    return get_impl()->audit();
  }
  __pure_function cxx11_constexpr std::size_t
  brutto_size() const cxx11_noexcept {
    return get_impl()->brutto_size();
  }
  __pure_function cxx11_constexpr std::size_t
  netto_size() const cxx11_noexcept {
    return get_impl()->netto_size();
  }
  __pure_function cxx11_constexpr std::size_t
  index_size() const cxx11_noexcept {
    return get_impl()->index_size();
  }
  __pure_function cxx11_constexpr std::size_t
  payload_size_bytes() const cxx11_noexcept {
    return get_impl()->payload_size_bytes();
  }
  __pure_function cxx11_constexpr std::size_t
  head_space() const cxx11_noexcept {
    return get_impl()->head_space();
  }
  __pure_function cxx11_constexpr std::size_t
  tail_space_units() const cxx11_noexcept {
    return get_impl()->tail_space_units();
  }
  __pure_function cxx11_constexpr std::size_t
  tail_space_bytes() const cxx11_noexcept {
    return get_impl()->tail_space_bytes();
  }
  __pure_function cxx11_constexpr std::size_t
  junk_units() const cxx11_noexcept {
    return get_impl()->junk_units();
  }
  __pure_function cxx11_constexpr std::size_t
  junk_bytes() const cxx11_noexcept {
    return get_impl()->junk_bytes();
  }
  __pure_function cxx11_constexpr bool empty() const cxx11_noexcept {
    return get_impl()->empty();
  }
  __pure_function cxx11_constexpr std::size_t capacity() const cxx11_noexcept {
    return get_impl()->capacity();
  }
  __pure_function cxx11_constexpr std::size_t
  loose_count() const cxx11_noexcept {
    return get_impl()->loose_count();
  }
  __pure_function cxx11_constexpr bool is_sorted() const cxx11_noexcept {
    return get_impl()->is_sorted();
  }
  __pure_function cxx11_constexpr bool have_preplaced() const cxx11_noexcept {
    return get_impl()->have_preplaced();
  }
  void ensure() { get_impl()->ensure(); }
  void debug_check() { get_impl()->debug_check(); }
  void reset() cxx11_noexcept { return get_impl()->reset(); }
  bool optimize(const details::tuple_rw::optimize_flags flags =
                    details::tuple_rw::optimize_flags::
                        all) /* returns true if iterators become invalid */ {
    return get_impl()->optimize(flags);
  }
  bool compactify() /* returns true if iterators become invalid */ {
    return get_impl()->compactify();
  }
  bool sort_index(
      bool force = false) /* returns true if iterators become invalid */ {
    return get_impl()->sort_index(force);
  }

  bool erase(const dynamic_collection_iterator_rw &it) {
    return get_impl()->erase(it);
  }
  std::size_t erase(const dynamic_collection_iterator_rw &from,
                    const dynamic_collection_iterator_rw &to) {
    return get_impl()->erase(from, to);
  }
  std::size_t erase(const dynamic_collection_rw &collection) {
    return get_impl()->erase(collection);
  }
  template <typename TOKEN> bool erase(const TOKEN &ident) {
    return get_impl()->erase(ident);
  }

  __pure_function tuple_ro_weak take_weak_asis() const {
    return tuple_ro_weak(get_impl()->take_asis());
  }

  /* возвращает пару, во втором элементе признак инвалидации итераторов */
  std::pair<tuple_ro_weak, bool> take_weak_optimized() {
    const auto pair = get_impl()->take_optimized();
    return std::make_pair(tuple_ro_weak(pair.first), pair.second);
  }

  /* возвращает пару, во втором элементе признак инвалидации итераторов */
  std::pair<tuple_ro_weak, bool> take_weak(bool dont_optimize = false) {
    return dont_optimize ? take_weak_optimized()
                         : std::make_pair(take_weak_asis(), false);
  }

  tuple_ro_managed move_to_ro(bool dont_optimize = false) {
    if (!dont_optimize)
      optimize();
    return tuple_ro_managed(std::move(self()));
  }

  __pure_function tuple_ro_managed take_managed_clone_asis(
      bool hollow_if_empty = false,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot()) const {
    const auto weak = take_weak_asis();
    if (hollow_if_empty && unlikely(weak.empty()))
      return tuple_ro_managed();

    return tuple_ro_managed::clone(weak, schema(), allot_tag);
  }

  /* возвращает пару, во втором элементе признак инвалидации итераторов */
  std::pair<tuple_ro_managed, bool> take_managed_clone_optimized(
      bool hollow_if_empty = false,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    const auto pair = take_weak_optimized();
    if (hollow_if_empty && unlikely(pair.first.empty()))
      return std::make_pair(tuple_ro_managed(), pair.second);

    return std::make_pair(
        tuple_ro_managed::clone(pair.first, schema(), allot_tag), pair.second);
  }

  /* возвращает пару, во втором элементе признак инвалидации итераторов */
  std::pair<tuple_ro_managed, bool> take_managed_clone(
      bool dont_optimize = false, bool hollow_if_empty = false,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return dont_optimize
               ? take_managed_clone_optimized(hollow_if_empty, allot_tag)
               : std::make_pair(
                     take_managed_clone_asis(hollow_if_empty, allot_tag),
                     false);
  }

  __pure_function static cxx11_constexpr size_t estimate_required_space(
      size_t loose_items_limit, std::size_t data_size_limit,
      const fptu::schema *schema, bool dont_account_preplaced = false) {
    return details::tuple_rw::estimate_required_space(
        loose_items_limit, data_size_limit, schema, dont_account_preplaced);
  }

  __pure_function static cxx11_constexpr std::size_t
  estimate_required_space(const tuple_ro_weak &ro, const std::size_t more_items,
                          const std::size_t more_payload,
                          const fptu::schema *schema) {
    return details::tuple_rw::estimate_required_space(ro.get_impl(), more_items,
                                                      more_payload, schema);
  }

  __pure_function static cxx11_constexpr std::size_t estimate_required_space(
      const tuple_ro_managed &ro, const std::size_t more_items,
      const std::size_t more_payload, const fptu::schema *schema) {
    return details::tuple_rw::estimate_required_space(ro.get_impl(), more_items,
                                                      more_payload, schema);
  }

  //----------------------------------------------------------------------------

  template <typename TOKEN>
  __pure_function inline details::tuple_rw::collection_rw<TOKEN>
  collection(const TOKEN &ident) {
    return get_impl()->collection(ident);
  }

  template <typename TOKEN>
  __pure_function inline details::tuple_rw::accessor_rw<TOKEN>
  operator[](const TOKEN &ident) {
    return get_impl()->operator[](ident);
  }

  __pure_function field_iterator_ro cbegin() const cxx11_noexcept {
    return get_impl()->cbegin(schema());
  }
  cxx11_constexpr field_iterator_ro cend() const cxx11_noexcept {
    return get_impl()->cend(schema());
  }
  __pure_function field_iterator_ro begin() const cxx11_noexcept {
    return cbegin();
  }
  cxx11_constexpr field_iterator_ro end() const cxx11_noexcept {
    return cend();
  }

  //----------------------------------------------------------------------------

  cxx11_constexpr loose_iterator_ro cbegin_loose() const cxx11_noexcept {
    return get_impl()->cbegin_loose();
  }
  cxx11_constexpr loose_iterator_ro cend_loose() const cxx11_noexcept {
    return get_impl()->cend_loose();
  }

  cxx11_constexpr loose_iterator_ro begin_loose() const cxx11_noexcept {
    return cbegin_loose();
  }
  cxx11_constexpr loose_iterator_ro end_loose() const cxx11_noexcept {
    return cend_loose();
  }
};

/* Управляемый R/W-кортеж с данными в управляемом буфере со счётчиком ссылок.
 * Но БЕЗ функционала увеличения размера нижележащего буфера, поэтому кидается
 * исключениями в случае нехватки места.
 *
 * Следует отметить, что допускается только монопольное использование как
 * экземпляра кортежа, так и буфера (несмотря на то, что буфер допускает
 * совместное чтение данных).
 * Реализация автоматической COW стратегии возможна на уровне управляемых
 * буферов, но это НЕ представляется ни необходимым, ни рациональным:
 *  - Требует проверки перед каждой операцией изменения кортежа;
 *  - Предполагает вероятность инвалидации итераторов как перед, так и после
 *    любой изменяющей кортеж операции. Проще говоря, не-const-итераторы
 *    становятся одноразовыми, а контроль их верного использования стоит
 *    дополнительных усилий.
 *  - Монопольное использование и клонирование данных при конструировании
 *    класса R/W-кортежа выглядит более эффективным и прозрачным.
 *
 * Таким образом, R/W-кортеж требует эксклюзивного владения буфером и при
 * необходимости клонирует его при получении.
 *
 * Требование монопольного использования буфера также определяет поведение при
 * получении сериализованной читаемой формы кортежа в управляемом буфере
 * в виде экземпляра tuple_ro_managed:
 *  - без копирования данных посредством std::move, с разрушением исходного
 *    R/W-представления. Технически это реализуется функцией move_to_ro(),
 *    а также перемещающим конструктором tuple_ro_managed из tuple_rw_fixed
 *  - с выделением нового буфера и копированием в него данных. Технически
 *    это реализуется функцией take_managed_clone(), а также копирующий
 *    конструктор tuple_ro_managed из tuple_rw_fixed. */
class FPTU_API_TYPE tuple_rw_fixed : public tuple_crtp_writer<tuple_rw_fixed> {
  friend class tuple_crtp_writer<tuple_rw_fixed>;
  friend class tuple_ro_managed;

protected:
  using impl = details::tuple_rw;
  impl *pimpl_;
  explicit cxx11_constexpr
  tuple_rw_fixed(details::tuple_rw *tuple) cxx11_noexcept : pimpl_(tuple) {}

public:
  void purge() {
    impl::deleter()(/* accepts nullptr */ pimpl_);
    pimpl_ = nullptr;
  }
  ~tuple_rw_fixed() { purge(); }

  tuple_rw_fixed(const tuple_rw_fixed &) = delete;
  tuple_rw_fixed &operator=(const tuple_rw_fixed &) = delete;

  void swap(tuple_rw_fixed &ditto) cxx11_noexcept {
    std::swap(pimpl_, ditto.pimpl_);
  }

  cxx14_constexpr tuple_rw_fixed(tuple_rw_fixed &&src) cxx11_noexcept
      : pimpl_(src.pimpl_) {
    src.pimpl_ = nullptr;
  }

  tuple_rw_fixed &operator=(tuple_rw_fixed &&src);

  tuple_rw_fixed(tuple_ro_managed &&src);

  tuple_rw_fixed &operator=(tuple_ro_managed &&src);

  tuple_rw_fixed(const defaults &);

  tuple_rw_fixed(
      const initiation_scale scale = initiation_scale::scale_default);

  tuple_rw_fixed(const initiation_scale scale, const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  tuple_rw_fixed(std::size_t items_limit, std::size_t data_bytes,
                 const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  tuple_rw_fixed(const fptu::details::audit_holes_info &holes_info,
                 const tuple_ro_weak &ro, const defaults &);

  tuple_rw_fixed(
      const fptu::details::audit_holes_info &holes_info,
      const tuple_ro_weak &ro,
      const initiation_scale scale = initiation_scale::scale_default);

  tuple_rw_fixed(const fptu::details::audit_holes_info &holes_info,
                 const tuple_ro_weak &ro, const initiation_scale scale,
                 const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  tuple_rw_fixed(const fptu::details::audit_holes_info &holes_info,
                 const tuple_ro_weak &ro, std::size_t more_items,
                 std::size_t more_payload, const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  tuple_rw_fixed(const void *source_ptr, std::size_t source_bytes,
                 const defaults &);

  tuple_rw_fixed(
      const void *source_ptr, std::size_t source_bytes,
      const initiation_scale scale = initiation_scale::scale_default);

  tuple_rw_fixed(const void *source_ptr, std::size_t source_bytes,
                 const initiation_scale scale, const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  tuple_rw_fixed(const void *source_ptr, std::size_t source_bytes,
                 std::size_t more_items, std::size_t more_payload,
                 const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  static tuple_rw_fixed clone(const tuple_ro_weak &src, const defaults &);

  static tuple_rw_fixed
  clone(const tuple_ro_weak &src,
        const initiation_scale scale = initiation_scale::scale_default);

  static tuple_rw_fixed
  clone(const tuple_ro_weak &src, const initiation_scale scale,
        const fptu::schema *schema,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  static tuple_rw_fixed
  clone(const tuple_ro_weak &src, std::size_t more_items,
        std::size_t more_payload, const fptu::schema *schema,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  static tuple_rw_fixed clone(const tuple_ro_managed &src, const defaults &);

  static tuple_rw_fixed
  clone(const tuple_ro_managed &src,
        const initiation_scale scale = initiation_scale::scale_default);

  static tuple_rw_fixed
  clone(const tuple_ro_managed &src, const initiation_scale scale,
        const fptu::schema *schema,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  static tuple_rw_fixed
  clone(const tuple_ro_managed &src, std::size_t more_items,
        std::size_t more_payload, const fptu::schema *schema,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  static tuple_rw_fixed
  clone(const tuple_rw_fixed &src,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  static tuple_rw_fixed
  clone(const tuple_rw_fixed &src, const initiation_scale scale,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  static tuple_rw_fixed
  clone(const tuple_rw_fixed &src, std::size_t more_items,
        std::size_t more_payload,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot());

  __pure_function static std::size_t
  estimate_required_space(const tuple_rw_fixed &rw,
                          const std::size_t more_items,
                          const std::size_t more_payload) {
    return details::tuple_rw::estimate_required_space(
        rw.index_size() + more_items, rw.payload_size_bytes() + more_payload,
        rw.schema());
  }

  __pure_function std::size_t
  estimate_required_space(const std::size_t more_items,
                          const std::size_t more_payload) const {
    return estimate_required_space(*this, more_items, more_payload);
  }

  bool operator==(const tuple_ro_weak &ditto) const cxx11_noexcept {
    return get_impl()->operator==(ditto.get_impl());
  }
  bool operator!=(const tuple_ro_weak &ditto) const cxx11_noexcept {
    return !get_impl()->operator==(ditto.get_impl());
  }
  bool operator==(const tuple_ro_managed &ditto) const cxx11_noexcept {
    return get_impl()->operator==(ditto.get_impl());
  }
  bool operator!=(const tuple_ro_managed &ditto) const cxx11_noexcept {
    return !get_impl()->operator==(ditto.get_impl());
  }
  bool operator==(const tuple_rw_fixed &ditto) const cxx11_noexcept {
    return get_impl() == ditto.get_impl();
  }
  bool operator!=(const tuple_rw_fixed &ditto) const cxx11_noexcept {
    return get_impl() != ditto.get_impl();
  }
};

/* Управляемый R/W-кортеж с данными в управляемом буфере со счётчиком ссылок,
 * С функционалом автоматического увеличения/перевыделения нижележащего буфера
 * при нехватки места.
 *
 * tuple_rw_managed перехватывает исключения выбрасываемые из-за нехватки места,
 * а затем:
 *  - выделяет новый буфер с увеличением размера (с использованием нескольких
 *    эвристик и информации из перехваченного исключения).
 *  - копирует данные в новый буфер.
 *  - освобождает старый буфер и перезапускает операцию, для которой не хватило
 *    места.
 *
 * Остальные свойства и поведение tuple_rw_managed аналогично tuple_rw_fixed. */
class FPTU_API_TYPE tuple_rw_managed : public tuple_rw_fixed {
  using base = tuple_rw_fixed;
  void manage_space_deficit(const insufficient_space &wanna);

public:
  tuple_rw_managed(const tuple_rw_managed &) = delete;
  tuple_rw_managed &operator=(const tuple_rw_managed &) = delete;

  tuple_rw_managed(tuple_ro_managed &&src) cxx11_noexcept
      : base(std::move(src)) {}

  tuple_rw_managed &operator=(tuple_ro_managed &&src) {
    base::operator=(std::move(src));
    return *this;
  }

  cxx14_constexpr tuple_rw_managed(tuple_rw_fixed &&src) cxx11_noexcept
      : base(std::move(src)) {}

  tuple_rw_managed &operator=(tuple_rw_fixed &&src) {
    base::operator=(std::move(src));
    return *this;
  }

  cxx14_constexpr tuple_rw_managed(tuple_rw_managed &&src) cxx11_noexcept
      : base(std::move(src)) {}

  tuple_rw_managed &operator=(tuple_rw_managed &&src) {
    base::operator=(std::move(src));
    return *this;
  }

  tuple_rw_managed(const defaults &ditto) : base(ditto) {}

  tuple_rw_managed(
      const initiation_scale scale = initiation_scale::scale_default)
      : base(scale) {}

  tuple_rw_managed(
      const initiation_scale scale, const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : base(scale, schema, allot_tag) {}

  tuple_rw_managed(
      std::size_t items_limit, std::size_t data_bytes,
      const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : base(items_limit, data_bytes, schema, allot_tag) {}

  tuple_rw_managed(const fptu::details::audit_holes_info &holes_info,
                   const tuple_ro_weak &ro, const defaults &ditto)
      : base(holes_info, ro, ditto) {}

  tuple_rw_managed(
      const fptu::details::audit_holes_info &holes_info,
      const tuple_ro_weak &ro,
      const initiation_scale scale = initiation_scale::scale_default)
      : base(holes_info, ro, scale) {}

  tuple_rw_managed(
      const fptu::details::audit_holes_info &holes_info,
      const tuple_ro_weak &ro, const initiation_scale scale,
      const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : base(holes_info, ro, scale, schema, allot_tag) {}

  tuple_rw_managed(
      const fptu::details::audit_holes_info &holes_info,
      const tuple_ro_weak &ro, std::size_t more_items, std::size_t more_payload,
      const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : base(holes_info, ro, more_items, more_payload, schema, allot_tag) {}

  tuple_rw_managed(const void *source_ptr, std::size_t source_bytes,
                   const defaults &ditto)
      : base(source_ptr, source_bytes, ditto) {}

  tuple_rw_managed(
      const void *source_ptr, std::size_t source_bytes,
      const initiation_scale scale = initiation_scale::scale_default)
      : base(source_ptr, source_bytes, scale) {}

  tuple_rw_managed(
      const void *source_ptr, std::size_t source_bytes,
      const initiation_scale scale, const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : base(source_ptr, source_bytes, scale, schema, allot_tag) {}

  tuple_rw_managed(
      const void *source_ptr, std::size_t source_bytes, std::size_t more_items,
      std::size_t more_payload, const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : base(source_ptr, source_bytes, more_items, more_payload, schema,
             allot_tag) {}

  static tuple_rw_managed clone(const tuple_ro_weak &src,
                                const defaults &ditto) {
    return base::clone(src, ditto);
  }

  static tuple_rw_managed
  clone(const tuple_ro_weak &src,
        const initiation_scale scale = initiation_scale::scale_default) {
    return base::clone(src, scale);
  }

  static tuple_rw_managed
  clone(const tuple_ro_weak &src, const initiation_scale scale,
        const fptu::schema *schema,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return base::clone(src, scale, schema, allot_tag);
  }

  static tuple_rw_managed
  clone(const tuple_ro_weak &src, std::size_t more_items,
        std::size_t more_payload, const fptu::schema *schema,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return base::clone(src, more_items, more_payload, schema, allot_tag);
  }

  static tuple_rw_managed clone(const tuple_ro_managed &src,
                                const defaults &ditto) {
    return base::clone(src, ditto);
  }

  static tuple_rw_managed
  clone(const tuple_ro_managed &src,
        const initiation_scale scale = initiation_scale::scale_default) {
    return base::clone(src, scale);
  }

  static tuple_rw_managed
  clone(const tuple_ro_managed &src, const initiation_scale scale,
        const fptu::schema *schema,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return base::clone(src, scale, schema, allot_tag);
  }

  static tuple_rw_managed
  clone(const tuple_ro_managed &src, std::size_t more_items,
        std::size_t more_payload, const fptu::schema *schema,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return base::clone(src, more_items, more_payload, schema, allot_tag);
  }

  static tuple_rw_managed
  clone(const tuple_rw_fixed &src,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return base::clone(src, allot_tag);
  }

  static tuple_rw_managed
  clone(const tuple_rw_fixed &src, const initiation_scale scale,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return base::clone(src, scale, allot_tag);
  }

  static tuple_rw_managed
  clone(const tuple_rw_fixed &src, std::size_t more_items,
        std::size_t more_payload,
        const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return base::clone(src, more_items, more_payload, allot_tag);
  }

#define HERE_THUNK_MAKE(VALUE_TYPE, NAME)                                      \
  FPTU_TEMPLATE_FOR_STATIC_TOKEN                                               \
  inline void set_##NAME(const TOKEN &ident, const VALUE_TYPE value) {         \
    while (true) {                                                             \
      try {                                                                    \
        return base::set_##NAME(ident, value);                                 \
      } catch (const insufficient_space &wanna) {                              \
        manage_space_deficit(wanna);                                           \
        continue;                                                              \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  void set_##NAME(const token &ident, const VALUE_TYPE value);                 \
  details::tuple_rw::collection_iterator_rw<token> insert_##NAME(              \
      const token &ident, const VALUE_TYPE value);

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
  HERE_THUNK_MAKE(int32_t, number)
  HERE_THUNK_MAKE(uint32_t, number)
  HERE_THUNK_MAKE(int64_t, number)
  HERE_THUNK_MAKE(uint64_t, number)
  HERE_THUNK_MAKE(float, number)
  HERE_THUNK_MAKE(double, number)
#undef HERE_THUNK_MAKE

  bool erase(const dynamic_collection_iterator_rw &it);
  std::size_t erase(const dynamic_collection_iterator_rw &from,
                    const dynamic_collection_iterator_rw &to);
  std::size_t erase(const dynamic_collection_rw &collection);
  bool erase(const token &ident);

  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  bool erase(const TOKEN &ident) {
    while (true) {
      try {
        return base::erase(ident);
      } catch (const insufficient_space &wanna) {
        manage_space_deficit(wanna);
        continue;
      }
    }
  }
};

//------------------------------------------------------------------------------

inline tuple_ro_weak::tuple_ro_weak(const tuple_ro_managed &body)
    : tuple_ro_weak(body.take_weak()) {}

inline tuple_ro_weak::tuple_ro_weak(const tuple_rw_fixed &body)
    : tuple_ro_weak(body.take_weak_asis()) {}

inline tuple_ro_weak::tuple_ro_weak(const tuple_rw_managed &body)
    : tuple_ro_weak(body.take_weak_asis()) {}

inline tuple_ro_weak /* preplaced_nested::value_type */
preplaced_nested::value_nothrow() const cxx11_noexcept {
  return unlikely(nil())
             ? tuple_ro_weak()
             : tuple_ro_weak(
                   erthink::constexpr_pointer_cast<const details::tuple_ro *>(
                       traits::read(payload())));
}

//------------------------------------------------------------------------------

inline bool
tuple_ro_weak::operator==(const tuple_ro_managed &ditto) const cxx11_noexcept {
  return ditto == *this;
}
inline bool
tuple_ro_weak::operator!=(const tuple_ro_managed &ditto) const cxx11_noexcept {
  return ditto != *this;
}
inline bool
tuple_ro_weak::operator==(const tuple_rw_fixed &ditto) const cxx11_noexcept {
  return ditto.operator==(*this);
}
inline bool
tuple_ro_weak::operator!=(const tuple_rw_fixed &ditto) const cxx11_noexcept {
  return ditto.operator!=(*this);
}

//------------------------------------------------------------------------------

__pure_function inline const fptu::schema *
tuple_ro_managed::peek_schema() const cxx11_noexcept {
  const details::tuple_rw *basis = peek_basis();
  return likely(basis) ? basis->schema_ : nullptr;
}

inline bool
tuple_ro_managed::operator==(const tuple_rw_fixed &ditto) const cxx11_noexcept {
  return ditto == *this;
}
inline bool
tuple_ro_managed::operator!=(const tuple_rw_fixed &ditto) const cxx11_noexcept {
  return ditto != *this;
}

} // namespace fptu

//------------------------------------------------------------------------------
/* Информация о версии и сборке */

typedef struct fptu_version_info {
  uint8_t major;
  uint8_t minor;
  uint16_t release;
  uint32_t revision;
  struct {
    const char *datetime;
    const char *tree;
    const char *commit;
    const char *describe;
  } git;
} fptu_version_info;

typedef struct fptu_build_info {
  const char *datetime;
  const char *target;
  const char *cmake_options;
  const char *compiler;
  const char *compile_flags;
} fptu_build_info;

#if HAVE_FPTU_VERSIONINFO
extern FPTU_API const fptu_version_info fptu_version;
#endif /* HAVE_FPTU_VERSIONINFO */
extern FPTU_API const fptu_build_info fptu_build;

//------------------------------------------------------------------------------
/* Сервисные функции (будет пополнятся). */

/* Функция обратного вызова, используемая для трансляции идентификаторов/тегов
 * полей в символические имена. Функция будет вызываться многократно, при
 * вывода имени каждого поля кортежа, включая все вложенные кортежи.
 *
 * Функция должна возвращать указатель на C-строку, значение которой будет
 * валидно до возврата из функции сериализации, либо до следующего вызова данной
 * функции.
 *
 * Если функция возвратит NULL, то при выводе соответствующего поля вместо
 * символического имени будет использован числовой идентификатор.
 *
 * Если функция возвратит указатель на пустую строку "", то соответствующее
 * поле будет пропущено. Таким образом, часть полей можно сделать "скрытыми"
 * для сериализации. */
typedef const char *(*fptu_tag2name_func)(const void *schema_ctx, unsigned tag);

/* Функция обратного вызова, используемая для трансляции значений полей типа
 * fptu_uint16 в символические имена enum-констант, в том числе true и false.
 * Функция будет вызываться многократно, при вывода каждого значения типа
 * fptu_uint16, включая все коллекции, массивы и поля вложенных кортежи.
 *
 * Функция должна возвращать указатель на C-строку, значение которой будет
 * валидно до возврата из функции сериализации, либо до следующего вызова данной
 * функции.
 *
 * Если функция возвратит NULL, то при выводе соответствующего значения вместо
 * символического имени будет использован числовой идентификатор.
 *
 * Если функция возвратит указатель на пустую строку "", то enum-значение
 * будет интерпретировано как bool - для нулевых значений будет
 * выведено "false", и "true" для отличных от нуля значений. */
typedef const char *(*fptu_value2enum_func)(const void *schema_ctx,
                                            unsigned tag, unsigned value);

enum fptu_json_options {
  fptu_json_default = 0,
  fptu_json_disable_JSON5 = 1 /* Выключает расширения JSON5 (больше кавычек) */,
  fptu_json_disable_Collections =
      2 /* Выключает поддержку коллекций:
           - При преобразовании в json коллекции НЕ будут выводиться
             как JSON-массивы, а соответствующие поля будут просто повторяться.
           - При преобразовании из json JSON-массивы не будут конвертироваться
             в коллекции, но вместо этого будет генерироваться
             ошибка несовпадения типов. */
  ,
  fptu_json_skip_NULLs = 4 /* TODO: Пропускать DENILs и пустые объекты */,
  fptu_json_sort_Tags = 8 /* TODO: Сортировать по тегам, иначе выводить в
                             порядке следования полей */
};
DEFINE_ENUM_FLAG_OPERATORS(fptu_json_options)

/* Функция обратного вызова для выталкивания сериализованных данных.
 * В случае успеха функция должна возвратить 0 (FPTU_SUCCESS), иначе код
 * ошибки.
 *
 * При преобразовании в JSON используется для вывода генерируемого текстового
 * представления из fptu_tuple2json(). */
typedef int (*fptu_emit_func)(void *emiter_ctx, const char *text,
                              size_t length);

#ifdef __cplusplus
//------------------------------------------------------------------------------
/* Вспомогательные функции и классы */

namespace fptu {

FPTU_API std::string format(const char *fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
    ;
FPTU_API std::string format_va(const char *fmt, va_list ap);
FPTU_API std::string hexadecimal(const void *data, std::size_t bytes);

} /* namespace fptu */

namespace std {
FPTU_API fptu::string_view to_string(const fptu::genus type);
} /* namespace std */

#endif /* __cplusplus */

#include "fast_positive/tuples/details/warnings_pop.h"
