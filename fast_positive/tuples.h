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

//------------------------------------------------------------------------------
#include "fast_positive/details/warnings_push_pt.h"

#include "fast_positive/details/api.h"
#include "fast_positive/details/types.h"

#ifdef __cplusplus

#include "fast_positive/details/warnings_push_system.h"

#include <cmath>     // for std::ldexp
#include <limits>    // for numeric_limits<>
#include <memory>    // for std::uniq_ptr
#include <ostream>   // for std::ostream
#include <stdexcept> // for std::invalid_argument
#include <string>    // for std::string

#include "fast_positive/details/warnings_pop.h"

#include "fast_positive/details/audit.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/exceptions.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/getter.h"
#include "fast_positive/details/legacy_common.h"
#include "fast_positive/details/ro.h"
#include "fast_positive/details/rw.h"
#include "fast_positive/details/schema.h"
#include "fast_positive/details/tagged_pointer.h"
#include "fast_positive/details/token.h"

#endif /* __cplusplus */

#include "fast_positive/details/warnings_pop.h"

//------------------------------------------------------------------------------

namespace fptu {

class schema /* Минималистический справочник схемы.
   Содержит необходимую для машины информацию об именах полей, их
   внутренних атрибутах и типе хранимых значений.

   С точки зрения API, главное назначение справочника схемы:
   трансляция символических ("человеческих") имен полей в "токены
   доступа".

   Полное определение в fast_positive/details/schema.h */
    ;

class token /* Токен доступа к полю кортежей.
  Является именем поля кортежа оттранслированным в координаты машинного
  представления схемы. В компактном и удобном для машины виде содержит всю
  необходимую информацию:
    - тип данных поля;
    - вид поля быстрое/медленное/коллекция или поле вложенной структуры;
    - смещение к данным для "быстрых" полей.
    и т.д.

  Полное определение в fast_positive/details/token.h */
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

/* Для удобного создания кортежей нужного масштаба */
enum initiation_scale {
  tiny /* 1/256 ≈1K */,
  small /* 1/64 ≈4K */,
  medium /* 1/16 ≈16K */,
  large /* 1/4 ≈64K */,
  extreme /* максимальный ≈256K */
};

/* Параметры по-умолчанию.
 * Пустотелая структура используется как теговый тип при вызове конструкторов.
 * Актуальные значения хранятся в статических членах внутри FPTU и
 * устанавливаются статическим методом. */
struct FPTU_API defaults {
  static std::unique_ptr<fptu::schema> schema;
  static hippeus::buffer_tag allot_tag;
  static initiation_scale scale;
  static void setup(const initiation_scale &scale,
                    std::unique_ptr<fptu::schema> &&schema,
                    const hippeus::buffer_tag &allot_tag);
};

//------------------------------------------------------------------------------

/* forward-декларация основных классов для взаимодействия между ними */
class tuple_ro_weak;
class tuple_ro_managed;
class tuple_rw_fixed;
class tuple_rw_managed;

/* аллокатор управляемых буферов по-умолчанию */
static inline hippeus::buffer_tag default_buffer_allot() noexcept {
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
  constexpr const TUPLE &self() const noexcept {
    return *static_cast<const TUPLE *>(this);
  }

public:
  __pure_function constexpr const details::tuple_ro *get_impl() const noexcept {
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
  HERE_CRTP_MAKE(const binary96_t &, get_b96)
  HERE_CRTP_MAKE(const binary128_t &, get_b128)
  HERE_CRTP_MAKE(const binary160_t &, get_b160)
  HERE_CRTP_MAKE(const binary192_t &, get_b192)
  HERE_CRTP_MAKE(const binary224_t &, get_b224)
  HERE_CRTP_MAKE(const binary256_t &, get_b256)
  HERE_CRTP_MAKE(const binary320_t &, get_b320)
  HERE_CRTP_MAKE(const binary384_t &, get_b384)
  HERE_CRTP_MAKE(const binary512_t &, get_b512)
  HERE_CRTP_MAKE(const ip_address_t &, get_ip_address)
  HERE_CRTP_MAKE(mac_address_t, get_mac_address)
  HERE_CRTP_MAKE(const ip_net_t &, get_ip_net)
  HERE_CRTP_MAKE(int64_t, get_integer)
  HERE_CRTP_MAKE(uint64_t, get_unsigned)
  HERE_CRTP_MAKE(double, get_float)
#undef HERE_CRTP_MAKE

  __pure_function explicit constexpr operator bool() const noexcept {
    return get_impl() != 0;
  }
  __pure_function const char *validate(const fptu::schema *schema,
                                       bool holes_are_not_allowed = false) const
      noexcept {
    return get_impl()->audit(schema, holes_are_not_allowed);
  }
  __pure_function static const char *
  validate(const void *ptr, std::size_t bytes, const fptu::schema *schema,
           bool holes_are_not_allowed = false) noexcept {
    return TUPLE::impl::audit(ptr, bytes, schema, holes_are_not_allowed);
  }
  __pure_function constexpr bool empty() const noexcept {
    return get_impl()->empty();
  }
  __pure_function constexpr std::size_t size() const noexcept {
    return get_impl()->size();
  }
  __pure_function constexpr const void *data() const noexcept {
    return get_impl()->data();
  }
  __pure_function constexpr std::size_t index_size() const noexcept {
    return get_impl()->index_size();
  }
  __pure_function constexpr bool is_sorted() const noexcept {
    return get_impl()->is_sorted();
  }
  __pure_function constexpr bool have_preplaced() const noexcept {
    return get_impl()->have_preplaced();
  }
  __pure_function operator iovec() const noexcept {
    return iovec(data(), size());
  }
};

class tuple_ro_weak : public tuple_crtp_reader<tuple_ro_weak>
/* Не-управляемый R/O-кортеж с данными в отдельном (внешнем) буфере.
  Фактически это просто указатель, поэтому кортеж
  валиден только пока доступны и не изменены данные во внешнем буфере.
  Может быть создан из кортежа любого класса (см далее). */
{
  friend class tuple_crtp_reader<tuple_ro_weak>;
  friend class tuple_ro_managed;
  template <typename> friend class tuple_crtp_writer;

protected:
  using impl = details::tuple_ro;
  const impl *pimpl_;
  explicit constexpr tuple_ro_weak(const impl *ptr) noexcept : pimpl_(ptr) {}

public:
  constexpr tuple_ro_weak() noexcept : tuple_ro_weak(nullptr) {}
  tuple_ro_weak(const void *ptr, std::size_t bytes, const fptu::schema *schema,
                bool skip_validation = false)
      : pimpl_(impl::make_from_buffer(ptr, bytes, schema, skip_validation)) {}
  ~tuple_ro_weak() =
      /* to foo was a literal type*/ default /* { pimpl_ = nullptr; } */;

  constexpr tuple_ro_weak(const tuple_ro_weak &) noexcept = default;
  tuple_ro_weak &operator=(const tuple_ro_weak &) noexcept = default;

  constexpr tuple_ro_weak(tuple_ro_weak &&src) noexcept = default;
  cxx14_constexpr tuple_ro_weak &operator=(tuple_ro_weak &&src) noexcept {
    pimpl_ = src.pimpl_;
    src.pimpl_ = nullptr;
    return *this;
  }
  void swap(tuple_ro_weak &ditto) noexcept { std::swap(pimpl_, ditto.pimpl_); }

  template <typename TOKEN>
  __pure_function inline tuple_ro_weak
  get_nested_weak(const TOKEN &ident) const {
    return tuple_ro_weak(get_impl()->get_nested(ident));
  }

  constexpr bool operator==(const tuple_ro_weak &ditto) const noexcept {
    return pimpl_ == ditto.pimpl_;
  }
  constexpr bool operator!=(const tuple_ro_weak &ditto) const noexcept {
    return pimpl_ != ditto.pimpl_;
  }
  inline bool operator==(const tuple_ro_managed &ditto) const noexcept;
  inline bool operator!=(const tuple_ro_managed &ditto) const noexcept;
  inline bool operator==(const tuple_rw_fixed &ditto) const noexcept;
  inline bool operator!=(const tuple_rw_fixed &ditto) const noexcept;
};

class tuple_ro_managed : public tuple_crtp_reader<tuple_ro_managed>
/* Управляемый R/O-кортеж с данными в управляемом буфере со счётчиком ссылок.
  Поддерживает std::move(), clone(), RAII для счётчика ссылок.
  Может быть создан из tuple_rw_managed без копирования данных, а из
  кортежей других классов с выделением буфера и полным копированием данных. */
{
  friend class tuple_crtp_reader<tuple_ro_managed>;
  template <typename TUPLE> friend class tuple_crtp_writer;

protected:
  using impl = details::tuple_ro;
  const impl *pimpl_;
  const hippeus::buffer *hb_;

  FPTU_API __cold __noreturn void throw_buffer_mismatch() const;
  void check_buffer() const {
    if (unlikely((pimpl_ != nullptr) != (hb_ != nullptr)))
      throw_buffer_mismatch();
    if (hb_) {
      if (unlikely(
              static_cast<const uint8_t *>(pimpl_->data()) < hb_->begin() ||
              static_cast<const uint8_t *>(pimpl_->data()) + pimpl_->size() >
                  hb_->end()))
        throw_buffer_mismatch();
    }
  }

  explicit tuple_ro_managed(const impl *ro,
                            const hippeus::buffer *buffer) noexcept
      : pimpl_(ro), hb_(buffer->add_reference(/* accepts nullptr */)) {
    check_buffer();
  }

  /* копирующие конструкторы с копированием данных */
  inline tuple_ro_managed(const tuple_ro_weak &,
                          const hippeus::buffer_tag &allot_tag);
  inline tuple_ro_managed(const tuple_ro_managed &,
                          const hippeus::buffer_tag &allot_tag);
  inline tuple_ro_managed(const tuple_rw_fixed &,
                          const hippeus::buffer_tag &allot_tag);

public:
  const hippeus::buffer *get_buffer() const noexcept { return hb_; }
  tuple_ro_managed() noexcept : pimpl_(nullptr), hb_(nullptr) {}
  void release() {
    hb_->detach(/* accepts nullptr */);
    pimpl_ = nullptr;
    hb_ = nullptr;
  }
  ~tuple_ro_managed() { release(); }

  tuple_ro_managed(
      const void *source_ptr, std::size_t source_bytes,
      const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : tuple_ro_managed() {
    /* TODO: конструктор из внешнего источника с выделением управляемого буфера
     * и копированием данных.
     *
     * Технически несложно выделить буфер и скопировать в него данные. Однако,
     * логично чтобы размер буфера и география размещения в нём данных полностью
     * совпадала с details::tuple_rw. Тогда это подзадача может быть выполнена
     * общим кодом, а главное можно предоставить перекрестные move и не-move
     * конструкторы между tuple_ro_managed и details::tuple_rw.
     *
     * При этом возникает два подспудных вопроса:
     *  - details::tuple_rw хранит указатель на схему внутри себя. Поэтому
     *    следует либо обеспечить полную совместимость с details::tuple_rw.
     *    Либо дополнительно требовать в перемещающих конструкторах R/W-форм
     *    указатель на схему, и проводить повторную проверку данных
     *    на соответствие схеме.
     *  - Если обеспечивать полную совместимость с details::tuple_rw, то встает
     *    вопрос о необходимости класса tuple_ro_managed как такового. Ведь
     *    проще использовать details::tuple_rw, скрыв изменяющие методы.
     *
     * Итоговое решение:
     *  - При создании tuple_ro_managed с выделением буфера и копированием
     *    в него данных производиться создание полноценного экземпляра
     *    details::tuple_rw, с последующим перемещением буфера в экземпляр
     *    tuple_ro_managed.
     *  - При создании tuple_ro_managed из существующего буфера (например при
     *    чтении вложенного кортежа) никаких дополнительных действий
     *    не предпринимается.
     *  - Перемещающие конструкторы R/W-форм из tuple_ro_managed должны получать
     *    указатель на схему, самостоятельно определять способ, которым был
     *    создан tuple_ro_managed и действовать соответствующим образом:
     *     1) Если буфер НЕ в монопольном использовании, то конструктор должен
     *        сделать копию данных вне зависимости от способа создание.
     *     2) Если буфер в монопольном использовании и R/O-форма была
     *        создана из details::tuple_rw, то внутри буфера, по смещению
     *        соответствующему полю details::tuple_rw::buffer_offset_ будет
     *        корректное смещение на сам буфер, а все поля schema_, head_, tail_
     *        и т.д. соответствовать R/O-форме. В этом случае конструктор
     *        может задействовать уже готовый, созданный ранее объект.
     *     3) При любом несоответствии должна быть выполнена проверка данных
     *        и создание объекта с "чистого листа". Однако, копирование данных
     *        должно производиться только при необходимости выделения нового
     *        буфера (если текущее расположение данных не позволяет разместить
     *        служебные данные details::tuple_rw). */
    FPTU_NOT_IMPLEMENTED();
    (void)source_ptr;
    (void)source_bytes;
    (void)schema;
    (void)allot_tag;
  }

  static tuple_ro_managed clone(const tuple_ro_weak &src,
                                const hippeus::buffer_tag &allot_tag =
                                    hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                        false)) {
    /* TODO: конструктор из уже проверенной R/O-формы с выделением управляющего
     * буфера.
     *
     * Здесь есть подспудный вопрос со схемой:
     *  - Схема не требуется для работы с R/O-формой, если считать что данные
     *    уже были проверены на корректность при создании tuple_ro_weak.
     *  - Если схему не передавать параметром, то скрытый/промежуточный
     *    экземпляр details::tuple_rw будет создан без схемы и тогда
     *    последующее создание R/W-форм приведет к повторной валидации данных.
     *  - Если же схему передавать, но не делать здесь повторной проверки
     *    данных, то может быть потенциальное катастрофическое несоответствие
     *    схемы и данных, которое приведет к неожиданному краху только
     *    если созданный экземпляр tuple_ro_managed будет преобразован
     *    в R/W-форму.
     *
     * Итоговое решение:
     *   - Не передаем схему и создаем скрытый/промежуточный экземпляр
     *     details::tuple_rw без схемы, но и без проверки данных.
     *   - При последующем преобразовании tuple_ro_managed в R/W-форму будет
     *     выполнена повторная проверка. Однако, считаем что такой сценарий
     *     достаточно редкий, либо пользователю следует сразу создавать
     *     R/W-форму из tuple_ro_weak. */
    return tuple_ro_managed(src, allot_tag);
  }

  static tuple_ro_managed clone(const tuple_ro_managed &src,
                                const hippeus::buffer_tag &allot_tag =
                                    hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                        false)) {
    return tuple_ro_managed(src, allot_tag);
  }
  static tuple_ro_managed clone(const tuple_rw_fixed &src,
                                const hippeus::buffer_tag &allot_tag =
                                    hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                        false)) {
    return tuple_ro_managed(src, allot_tag);
  }

  tuple_ro_managed(const tuple_ro_weak &nested_weak,
                   const tuple_ro_managed &managed_master)
      : tuple_ro_managed(nested_weak ? nested_weak.get_impl() : nullptr,
                         nested_weak ? managed_master.get_buffer() : nullptr) {}

  tuple_ro_managed(const tuple_ro_managed &src)
      : tuple_ro_managed(src.pimpl_, src.hb_) {}

  tuple_ro_managed &operator=(const tuple_ro_managed &src) {
    if (likely(hb_ != src.hb_)) {
      release();
      hb_ = src.hb_->add_reference();
    }
    pimpl_ = src.pimpl_;
    check_buffer();
    return *this;
  }

  inline tuple_ro_managed(tuple_rw_fixed &&src);
  tuple_ro_managed(tuple_ro_managed &&src) : pimpl_(src.pimpl_), hb_(src.hb_) {
    check_buffer();
    src.pimpl_ = nullptr;
    src.hb_ = nullptr;
  }

  tuple_ro_managed &operator=(tuple_ro_managed &&src) {
    release();
    hb_ = src.hb_;
    pimpl_ = src.pimpl_;
    check_buffer();
    src.pimpl_ = nullptr;
    src.hb_ = nullptr;
    return *this;
  }

  void swap(tuple_ro_managed &ditto) noexcept {
    std::swap(pimpl_, ditto.pimpl_);
    std::swap(hb_, ditto.hb_);
  }

  tuple_ro_weak take_weak() const noexcept { return tuple_ro_weak(pimpl_); }

  bool operator==(const tuple_ro_weak &ditto) const noexcept {
    return pimpl_ == ditto.pimpl_;
  }
  bool operator!=(const tuple_ro_weak &ditto) const noexcept {
    return pimpl_ != ditto.pimpl_;
  }
  bool operator==(const tuple_ro_managed &ditto) const noexcept {
    return pimpl_ == ditto.pimpl_;
  }
  bool operator!=(const tuple_ro_managed &ditto) const noexcept {
    return pimpl_ != ditto.pimpl_;
  }
  inline bool operator==(const tuple_rw_fixed &ditto) const noexcept;
  inline bool operator!=(const tuple_rw_fixed &ditto) const noexcept;

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
  cxx14_constexpr const TUPLE &self() const noexcept {
    return *static_cast<const TUPLE *>(this);
  }
  cxx14_constexpr TUPLE &self() noexcept { return *static_cast<TUPLE *>(this); }

protected:
  cxx14_constexpr details::tuple_rw *get_impl() noexcept {
    return self().pimpl_;
  }

public:
  cxx14_constexpr const details::tuple_rw *get_impl() const noexcept {
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
  HERE_CRTP_MAKE(const binary96_t &, get_b96)
  HERE_CRTP_MAKE(const binary128_t &, get_b128)
  HERE_CRTP_MAKE(const binary160_t &, get_b160)
  HERE_CRTP_MAKE(const binary192_t &, get_b192)
  HERE_CRTP_MAKE(const binary224_t &, get_b224)
  HERE_CRTP_MAKE(const binary256_t &, get_b256)
  HERE_CRTP_MAKE(const binary320_t &, get_b320)
  HERE_CRTP_MAKE(const binary384_t &, get_b384)
  HERE_CRTP_MAKE(const binary512_t &, get_b512)
  HERE_CRTP_MAKE(const ip_address_t &, get_ip_address)
  HERE_CRTP_MAKE(mac_address_t, get_mac_address)
  HERE_CRTP_MAKE(const ip_net_t &, get_ip_net)
  HERE_CRTP_MAKE(int64_t, get_integer)
  HERE_CRTP_MAKE(uint64_t, get_unsigned)
  HERE_CRTP_MAKE(double, get_float)
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
  inline details::tuple_rw::iterator_rw<token> insert_##NAME(                  \
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
  HERE_CRTP_MAKE(binary96_t &, b96)
  HERE_CRTP_MAKE(binary128_t &, b128)
  HERE_CRTP_MAKE(binary160_t &, b160)
  HERE_CRTP_MAKE(binary192_t &, b192)
  HERE_CRTP_MAKE(binary224_t &, b224)
  HERE_CRTP_MAKE(binary256_t &, b256)
  HERE_CRTP_MAKE(binary320_t &, b320)
  HERE_CRTP_MAKE(binary384_t &, b384)
  HERE_CRTP_MAKE(binary512_t &, b512)
  HERE_CRTP_MAKE(ip_address_t &, ip_address)
  HERE_CRTP_MAKE(mac_address_t, mac_address)
  HERE_CRTP_MAKE(ip_net_t &, ip_net)
  HERE_CRTP_MAKE(int64_t, integer)
  HERE_CRTP_MAKE(uint64_t, integer)
  HERE_CRTP_MAKE(int64_t, unsigned)
  HERE_CRTP_MAKE(uint64_t, unsigned)
  HERE_CRTP_MAKE(double, float)
#undef HERE_CRTP_MAKE

  template <typename TOKEN>
  inline void set_nested(const TOKEN &ident, const tuple_ro_weak &value) {
    get_impl()->set_nested(ident, value.get_impl());
  }
  details::tuple_rw::iterator_rw<token>
  insert_nested(const token &ident, const tuple_ro_weak &value) {
    return get_impl()->insert_nested(ident, value.get_impl());
  }

  __pure_function constexpr const hippeus::buffer *get_buffer() const noexcept {
    return get_impl()->get_buffer();
  }
  __pure_function explicit constexpr operator bool() const noexcept {
    return get_impl() != 0;
  }
  __pure_function const char *validate() const noexcept {
    return get_impl()->audit();
  }
  __pure_function constexpr std::size_t brutto_size() const noexcept {
    return get_impl()->brutto_size();
  }
  __pure_function constexpr std::size_t netto_size() const noexcept {
    return get_impl()->netto_size();
  }
  __pure_function constexpr std::size_t index_size() const noexcept {
    return get_impl()->index_size();
  }
  __pure_function constexpr std::size_t loose_count() const noexcept {
    return get_impl()->loose_count();
  }
  __pure_function constexpr bool is_sorted() const noexcept {
    return get_impl()->is_sorted();
  }
  __pure_function constexpr bool have_preplaced() const noexcept {
    return get_impl()->have_preplaced();
  }
  void ensure() { get_impl()->ensure(); }
  void debug_check() { get_impl()->debug_check(); }
  void reset() noexcept { return get_impl()->reset(); }
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

  bool erase(const dynamic_iterator_rw &it) { return get_impl()->erase(it); }
  std::size_t erase(const dynamic_iterator_rw &from,
                    const dynamic_iterator_rw &to) {
    return get_impl()->erase(from, to);
  }
  std::size_t erase(const dynamic_collection_rw &collection) {
    return get_impl()->erase(collection);
  }
  template <typename TOKEN> bool erase(const TOKEN &ident) {
    return get_impl()->erase(ident);
  }

  __pure_function constexpr std::size_t head_space() const noexcept {
    return get_impl()->head_space();
  }
  __pure_function constexpr std::size_t tail_space() const noexcept {
    return get_impl()->tail_space();
  }
  __pure_function constexpr std::size_t junk_space() const noexcept {
    return get_impl()->junk_space();
  }
  __pure_function constexpr bool empty() const noexcept {
    return get_impl()->empty();
  }

  __pure_function tuple_ro_weak take_weak_asis() const {
    return tuple_ro_weak(get_impl()->take_asis());
  }
  /* возвращает пару, во втором элементе признак инвалидации итераторв */
  std::pair<tuple_ro_weak, bool> take_weak_optimized() {
    const auto pair = get_impl()->take_optimized();
    return std::make_pair(tuple_ro_weak(pair.first), pair.second);
  }
  /* возвращает пару, во втором элементе признак инвалидации итераторв */
  std::pair<tuple_ro_weak, bool> take_weak(bool dont_optimize = false) {
    return dont_optimize ? take_weak_optimized()
                         : std::make_pair(take_weak_asis(), false);
  }

  __pure_function tuple_ro_managed move_to_ro(bool dont_optimize = false) {
    if (!dont_optimize)
      optimize();
    return tuple_ro_managed(std::move(self()));
  }

  /* возвращает пару, во втором элементе признак инвалидации итераторв */
  __pure_function tuple_ro_managed take_managed_clone_asis(
      bool hollow_if_empty = false,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot()) const {
    const auto weak = take_weak_asis();
    if (hollow_if_empty && unlikely(weak.empty()))
      return tuple_ro_managed();

    return tuple_ro_managed(weak, allot_tag);
  }
  /* возвращает пару, во втором элементе признак инвалидации итераторв */
  std::pair<tuple_ro_managed, bool> take_managed_clone_optimized(
      bool hollow_if_empty = false,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    const auto pair = take_weak_optimized();
    if (hollow_if_empty && unlikely(pair.first.empty()))
      return std::make_pair(tuple_ro_managed(), pair.second);

    return std::make_pair(tuple_ro_managed(pair.first, allot_tag), pair.second);
  }
  /* возвращает пару, во втором элементе признак инвалидации итераторв */
  std::pair<tuple_ro_managed, bool> take_managed_clone(
      bool dont_optimize = false, bool hollow_if_empty = false,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot()) {
    return dont_optimize
               ? take_managed_clone_optimized(hollow_if_empty, allot_tag)
               : std::make_pair(
                     take_managed_clone_asis(hollow_if_empty, allot_tag),
                     false);
  }

  __pure_function static constexpr size_t
  estimate_required_space(size_t loose_items_limit, std::size_t data_size_limit,
                          const fptu::schema *schema,
                          bool dont_account_preplaced = false) {
    return details::tuple_rw::estimate_required_space(
        loose_items_limit, data_size_limit, schema, dont_account_preplaced);
  }

  __pure_function static constexpr std::size_t
  estimate_required_space(const tuple_ro_weak &ro, const std::size_t more_items,
                          const std::size_t more_payload,
                          const fptu::schema *schema) {
    return details::tuple_rw::estimate_required_space(ro.get_impl(), more_items,
                                                      more_payload, schema);
  }

  __pure_function static constexpr std::size_t estimate_required_space(
      const tuple_ro_managed &ro, const std::size_t more_items,
      const std::size_t more_payload, const fptu::schema *schema) {
    return details::tuple_rw::estimate_required_space(ro.get_impl(), more_items,
                                                      more_payload, schema);
  }

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
class FPTU_API tuple_rw_fixed : public tuple_crtp_writer<tuple_rw_fixed> {
  friend class tuple_crtp_writer<tuple_rw_fixed>;
  friend class tuple_ro_managed;

protected:
  using impl = details::tuple_rw;
  impl *pimpl_;
  __cold __noreturn void throw_managed_buffer_required() const;

  inline tuple_rw_fixed(const tuple_ro_weak &src,
                        const hippeus::buffer_tag &allot_tag);
  inline tuple_rw_fixed(const tuple_ro_managed &src,
                        const hippeus::buffer_tag &allot_tag);
  inline tuple_rw_fixed(const tuple_rw_fixed &src,
                        const hippeus::buffer_tag &allot_tag);

public:
  tuple_rw_fixed(const defaults &);
  tuple_rw_fixed(const initiation_scale = initiation_scale::tiny);
  tuple_rw_fixed(const initiation_scale, const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot());
  void release() {
    impl::deleter()(/* accepts nullptr */ pimpl_);
    pimpl_ = nullptr;
  }
  ~tuple_rw_fixed() { release(); }

  tuple_rw_fixed(std::size_t items_limit, std::size_t data_bytes,
                 const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : pimpl_(impl::create_new(items_limit, data_bytes, schema, allot_tag)) {}

  tuple_rw_fixed(const fptu::details::audit_holes_info &holes_info,
                 const tuple_ro_weak &ro, std::size_t more_items,
                 std::size_t more_payload, const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : pimpl_(impl::create_from_ro(holes_info, ro.get_impl(), more_items,
                                    more_payload, schema, allot_tag)) {}

  tuple_rw_fixed(const void *source_ptr, std::size_t source_bytes,
                 std::size_t more_items, std::size_t more_payload,
                 const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : pimpl_(impl::create_from_buffer(source_ptr, source_bytes, more_items,
                                        more_payload, schema, allot_tag)) {}

  tuple_rw_fixed(const tuple_rw_fixed &) = delete;
  tuple_rw_fixed &operator=(const tuple_rw_fixed &) = delete;

  void swap(tuple_rw_fixed &ditto) noexcept { std::swap(pimpl_, ditto.pimpl_); }

  cxx14_constexpr tuple_rw_fixed(tuple_rw_fixed &&src) noexcept
      : pimpl_(src.pimpl_) {
    src.pimpl_ = nullptr;
  }
  tuple_rw_fixed &operator=(tuple_rw_fixed &&src) {
    if (likely(pimpl_ != src.pimpl_)) {
      release();
      pimpl_ = src.pimpl_;
      src.pimpl_ = nullptr;
    }
    return *this;
  }

  inline tuple_rw_fixed(tuple_ro_managed &&src);

  static tuple_rw_fixed clone(const tuple_ro_weak &src,
                              const hippeus::buffer_tag &allot_tag =
                                  hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                      false)) {
    return tuple_rw_fixed(src, allot_tag);
  }

  static tuple_rw_fixed clone(const tuple_ro_managed &src,
                              const hippeus::buffer_tag &allot_tag =
                                  hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                      false)) {
    return tuple_rw_fixed(src, allot_tag);
  }
  static tuple_rw_fixed clone(const tuple_rw_fixed &src,
                              const hippeus::buffer_tag &allot_tag =
                                  hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                      false)) {
    return tuple_rw_fixed(src, allot_tag);
  }

  bool operator==(const tuple_ro_weak &ditto) const noexcept {
    return get_impl()->operator==(ditto.get_impl());
  }
  bool operator!=(const tuple_ro_weak &ditto) const noexcept {
    return !get_impl()->operator==(ditto.get_impl());
  }
  bool operator==(const tuple_ro_managed &ditto) const noexcept {
    return get_impl()->operator==(ditto.get_impl());
  }
  bool operator!=(const tuple_ro_managed &ditto) const noexcept {
    return !get_impl()->operator==(ditto.get_impl());
  }
  bool operator==(const tuple_rw_fixed &ditto) const noexcept {
    return get_impl() == ditto.get_impl();
  }
  bool operator!=(const tuple_rw_fixed &ditto) const noexcept {
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
class FPTU_API tuple_rw_managed : public tuple_rw_fixed {
  using base = tuple_rw_fixed;
  void expand_underlying_buffer(const insufficient_space &deficit);

protected:
  tuple_rw_managed(const tuple_ro_weak &src,
                   const hippeus::buffer_tag &allot_tag)
      : base(src, allot_tag) {}

  tuple_rw_managed(const tuple_ro_managed &src,
                   const hippeus::buffer_tag &allot_tag)
      : base(src, allot_tag) {}

  tuple_rw_managed(const tuple_rw_fixed &src,
                   const hippeus::buffer_tag &allot_tag)
      : base(src, allot_tag) {}

public:
  tuple_rw_managed(const defaults &ditto) : base(ditto) {}
  tuple_rw_managed(const initiation_scale scale = initiation_scale::tiny)
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

  tuple_rw_managed(
      const fptu::details::audit_holes_info &holes_info,
      const tuple_ro_weak &ro, std::size_t more_items, std::size_t more_payload,
      const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : base(holes_info, ro, more_items, more_payload, schema, allot_tag) {}

  tuple_rw_managed(
      const void *source_ptr, std::size_t source_bytes, std::size_t more_items,
      std::size_t more_payload, const fptu::schema *schema,
      const hippeus::buffer_tag &allot_tag = default_buffer_allot())
      : base(source_ptr, source_bytes, more_items, more_payload, schema,
             allot_tag) {}

  static tuple_rw_managed clone(const tuple_ro_weak &src,
                                const hippeus::buffer_tag &allot_tag =
                                    hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                        false)) {
    return tuple_rw_managed(src, allot_tag);
  }

  tuple_rw_managed(const tuple_rw_managed &) = delete;
  tuple_rw_managed &operator=(const tuple_rw_managed &) = delete;

  tuple_rw_managed(tuple_ro_managed &&src) noexcept : base(std::move(src)) {}
  tuple_rw_managed &operator=(tuple_ro_managed &&src) {
    base::operator=(std::move(src));
    return *this;
  }

  cxx14_constexpr tuple_rw_managed(tuple_rw_fixed &&src) noexcept
      : base(std::move(src)) {}
  tuple_rw_managed &operator=(tuple_rw_fixed &&src) {
    base::operator=(std::move(src));
    return *this;
  }

  cxx14_constexpr tuple_rw_managed(tuple_rw_managed &&src) noexcept
      : base(std::move(src)) {}
  tuple_rw_managed &operator=(tuple_rw_managed &&src) {
    base::operator=(std::move(src));
    return *this;
  }

  static tuple_rw_managed clone(const tuple_ro_managed &src,
                                const hippeus::buffer_tag &allot_tag =
                                    hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                        false)) {
    return tuple_rw_managed(src, allot_tag);
  }
  static tuple_rw_managed clone(const tuple_rw_fixed &src,
                                const hippeus::buffer_tag &allot_tag =
                                    hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                        false)) {
    return tuple_rw_managed(src, allot_tag);
  }

#define HERE_THUNK_MAKE(VALUE_TYPE, NAME)                                      \
  FPTU_TEMPLATE_FOR_STATIC_TOKEN                                               \
  inline void set_##NAME(const TOKEN &ident, const VALUE_TYPE value) {         \
    while (true) {                                                             \
      try {                                                                    \
        return base::set_##NAME(ident, value);                                 \
      } catch (const insufficient_space &deficit) {                            \
        expand_underlying_buffer(deficit);                                     \
        continue;                                                              \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  void set_##NAME(const token &ident, const VALUE_TYPE value);                 \
  details::tuple_rw::iterator_rw<token> insert_##NAME(const token &ident,      \
                                                      const VALUE_TYPE value);

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

  bool erase(const dynamic_iterator_rw &it);
  std::size_t erase(const dynamic_iterator_rw &from,
                    const dynamic_iterator_rw &to);
  std::size_t erase(const dynamic_collection_rw &collection);
  bool erase(const token &ident);

  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  bool erase(const TOKEN &ident) {
    while (true) {
      try {
        return base::erase(ident);
      } catch (const insufficient_space &deficit) {
        expand_underlying_buffer(deficit);
        continue;
      }
    }
  }
};

//------------------------------------------------------------------------------

inline tuple_ro_managed::tuple_ro_managed(const tuple_ro_weak &src,
                                          const hippeus::buffer_tag &allot_tag)
    : tuple_ro_managed() {
  FPTU_NOT_IMPLEMENTED();
  (void)src;
  (void)allot_tag;
}
inline tuple_ro_managed::tuple_ro_managed(const tuple_ro_managed &src,
                                          const hippeus::buffer_tag &allot_tag)
    : tuple_ro_managed() {
  FPTU_NOT_IMPLEMENTED();
  (void)src;
  (void)allot_tag;
}
inline tuple_ro_managed::tuple_ro_managed(const tuple_rw_fixed &src,
                                          const hippeus::buffer_tag &allot_tag)
    : tuple_ro_managed() {
  FPTU_NOT_IMPLEMENTED();
  (void)src;
  (void)allot_tag;
}

inline tuple_ro_managed::tuple_ro_managed(tuple_rw_fixed &&src)
    : tuple_ro_managed() {
  FPTU_NOT_IMPLEMENTED();
  (void)src;
}

//------------------------------------------------------------------------------

inline bool tuple_ro_weak::operator==(const tuple_ro_managed &ditto) const
    noexcept {
  return ditto == *this;
}
inline bool tuple_ro_weak::operator!=(const tuple_ro_managed &ditto) const
    noexcept {
  return ditto != *this;
}
inline bool tuple_ro_weak::operator==(const tuple_rw_fixed &ditto) const
    noexcept {
  return ditto.operator==(*this);
}
inline bool tuple_ro_weak::operator!=(const tuple_rw_fixed &ditto) const
    noexcept {
  return ditto.operator!=(*this);
}

//------------------------------------------------------------------------------

inline bool tuple_ro_managed::operator==(const tuple_rw_fixed &ditto) const
    noexcept {
  return ditto == *this;
}
inline bool tuple_ro_managed::operator!=(const tuple_rw_fixed &ditto) const
    noexcept {
  return ditto != *this;
}

//------------------------------------------------------------------------------

inline tuple_rw_fixed::tuple_rw_fixed(const tuple_ro_weak &src,
                                      const hippeus::buffer_tag &allot_tag)
    : tuple_rw_fixed() {
  FPTU_NOT_IMPLEMENTED();
  (void)src;
  (void)allot_tag;
}
inline tuple_rw_fixed::tuple_rw_fixed(const tuple_ro_managed &src,
                                      const hippeus::buffer_tag &allot_tag) {
  FPTU_NOT_IMPLEMENTED();
  (void)src;
  (void)allot_tag;
}
inline tuple_rw_fixed::tuple_rw_fixed(const tuple_rw_fixed &src,
                                      const hippeus::buffer_tag &allot_tag) {
  FPTU_NOT_IMPLEMENTED();
  (void)src;
  (void)allot_tag;
}
inline tuple_rw_fixed::tuple_rw_fixed(tuple_ro_managed &&src)
    : tuple_rw_fixed() {
  FPTU_NOT_IMPLEMENTED();
  (void)src;
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
/* Сервисные функции и классы для C++ (будет пополнятся). */

namespace fptu {

FPTU_API std::string format(const char *fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 1, 2)))
#endif
    ;
FPTU_API std::string format_va(const char *fmt, va_list ap);
FPTU_API std::string hexadecimal(const void *data, std::size_t bytes);

} /* namespace fptu */

#endif /* __cplusplus */
