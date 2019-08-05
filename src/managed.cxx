/*
 *  Fast Positive Tuples (libfptu), aka Позитивные Кортежи
 *  Copyright 2016-2019 Leonid Yuriev <leo@yuriev.ru>
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

#include "fast_positive/tuples/internal.h"

namespace fptu {

std::unique_ptr<fptu::schema> defaults::schema;
hippeus::buffer_tag defaults::allot_tag = hippeus::default_allot_tag();
initiation_scale defaults::scale = tiny;

void defaults::setup(const fptu::initiation_scale &_scale,
                     std::unique_ptr<fptu::schema> &&_schema,
                     const hippeus::buffer_tag &_allot_tag) {
  defaults::scale = _scale;
  defaults::schema = std::move(_schema);
  defaults::allot_tag = _allot_tag ? _allot_tag : hippeus::default_allot_tag();
}

static const unsigned scale2items[unsigned(initiation_scale::extreme) + 1] = {
    (max_fields + 1) / 256 /* 32 */,
    (max_fields + 1) / 64 /* 128 */,
    (max_fields + 1) / 16 /* 512 */,
    (max_fields + 1) / 4 /* 2K */,
    max_fields /* 8K */,
};

static const unsigned scale2bytes[unsigned(initiation_scale::extreme) + 1] = {
    max_tuple_bytes_netto / 256 /* 1K */, max_tuple_bytes_netto / 64 /* 4K */,
    max_tuple_bytes_netto / 16 /* 16K */, max_tuple_bytes_netto / 4 /* 64K */,
    max_tuple_bytes_netto /* 256K */,
};

static const unsigned scale2more[unsigned(initiation_scale::extreme) + 1] = {
    unsigned(max_tuple_bytes_netto - details::units2bytes(max_fields)) / 256,
    unsigned(max_tuple_bytes_netto - details::units2bytes(max_fields)) / 64,
    unsigned(max_tuple_bytes_netto - details::units2bytes(max_fields)) / 16,
    unsigned(max_tuple_bytes_netto - details::units2bytes(max_fields)) / 4,
    unsigned(max_tuple_bytes_netto - details::units2bytes(max_fields)),
};

std::size_t estimate_space_for_tuple(const initiation_scale &scale,
                                     const fptu::schema *schema) {
  const size_t preplaced_bytes = schema ? schema->preplaced_bytes() : 0;
  const size_t scale_bytes = scale2bytes[scale];
  size_t data_bytes = scale_bytes - std::min(scale_bytes, preplaced_bytes);
  return details::tuple_rw::estimate_required_space(scale2items[scale],
                                                    data_bytes, schema);
}

//------------------------------------------------------------------------------

tuple_rw_fixed::tuple_rw_fixed(const defaults &)
    : tuple_rw_fixed(defaults::scale, defaults::schema.get(),
                     defaults::allot_tag) {}
tuple_rw_fixed::tuple_rw_fixed(const initiation_scale scale)
    : tuple_rw_fixed(scale, defaults::schema.get(), defaults::allot_tag) {}
tuple_rw_fixed::tuple_rw_fixed(const initiation_scale scale,
                               const fptu::schema *schema,
                               const hippeus::buffer_tag &allot_tag)
    : tuple_rw_fixed(scale2items[scale], scale2bytes[scale], schema,
                     allot_tag) {}
tuple_rw_fixed::tuple_rw_fixed(std::size_t items_limit, std::size_t data_bytes,
                               const fptu::schema *schema,
                               const hippeus::buffer_tag &allot_tag)
    : pimpl_(impl::create_new(items_limit, data_bytes, schema, allot_tag)) {}

tuple_rw_fixed::tuple_rw_fixed(
    const fptu::details::audit_holes_info &holes_info, const tuple_ro_weak &ro,
    const defaults &)
    : tuple_rw_fixed(holes_info, ro, defaults::scale, defaults::schema.get(),
                     defaults::allot_tag) {}
tuple_rw_fixed::tuple_rw_fixed(
    const fptu::details::audit_holes_info &holes_info, const tuple_ro_weak &ro,
    const initiation_scale scale)
    : tuple_rw_fixed(holes_info, ro, scale, defaults::schema.get(),
                     defaults::allot_tag) {}
tuple_rw_fixed::tuple_rw_fixed(
    const fptu::details::audit_holes_info &holes_info, const tuple_ro_weak &ro,
    const initiation_scale scale, const fptu::schema *schema,
    const hippeus::buffer_tag &allot_tag)
    : tuple_rw_fixed(holes_info, ro, scale2items[scale], scale2more[scale],
                     schema, allot_tag) {}
tuple_rw_fixed::tuple_rw_fixed(
    const fptu::details::audit_holes_info &holes_info, const tuple_ro_weak &ro,
    std::size_t more_items, std::size_t more_payload,
    const fptu::schema *schema, const hippeus::buffer_tag &allot_tag)
    : pimpl_(impl::create_from_ro(holes_info, ro.get_impl(), more_items,
                                  more_payload, schema, allot_tag)) {}

tuple_rw_fixed::tuple_rw_fixed(const void *source_ptr, std::size_t source_bytes,
                               const defaults &)
    : tuple_rw_fixed(source_ptr, source_bytes, defaults::scale,
                     defaults::schema.get(), defaults::allot_tag) {}
tuple_rw_fixed::tuple_rw_fixed(const void *source_ptr, std::size_t source_bytes,
                               const initiation_scale scale)
    : tuple_rw_fixed(source_ptr, source_bytes, scale, defaults::schema.get(),
                     defaults::allot_tag) {}
tuple_rw_fixed::tuple_rw_fixed(const void *source_ptr, std::size_t source_bytes,
                               const initiation_scale scale,
                               const fptu::schema *schema,
                               const hippeus::buffer_tag &allot_tag)
    : tuple_rw_fixed(source_ptr, source_bytes, scale2items[scale],
                     scale2more[scale], schema, allot_tag) {}
tuple_rw_fixed::tuple_rw_fixed(const void *source_ptr, std::size_t source_bytes,
                               std::size_t more_items, std::size_t more_payload,
                               const fptu::schema *schema,
                               const hippeus::buffer_tag &allot_tag)
    : pimpl_(impl::create_from_buffer(source_ptr, source_bytes, more_items,
                                      more_payload, schema, allot_tag)) {}

tuple_rw_fixed tuple_rw_fixed::clone(const tuple_ro_weak &src,
                                     const defaults &ditto) {
  return tuple_rw_fixed(src ? src.data() : nullptr, src ? src.size() : 0,
                        ditto);
}
tuple_rw_fixed tuple_rw_fixed::clone(const tuple_ro_weak &src,
                                     const initiation_scale scale) {
  return tuple_rw_fixed(src ? src.data() : nullptr, src ? src.size() : 0,
                        scale);
}
tuple_rw_fixed tuple_rw_fixed::clone(const tuple_ro_weak &src,
                                     const initiation_scale scale,
                                     const fptu::schema *schema,
                                     const hippeus::buffer_tag &allot_tag) {
  return tuple_rw_fixed(src ? src.data() : nullptr, src ? src.size() : 0, scale,
                        schema, allot_tag);
}
tuple_rw_fixed tuple_rw_fixed::clone(const tuple_ro_weak &src,
                                     std::size_t more_items,
                                     std::size_t more_payload,
                                     const fptu::schema *schema,
                                     const hippeus::buffer_tag &allot_tag) {
  return tuple_rw_fixed(src ? src.data() : nullptr, src ? src.size() : 0,
                        more_items, more_payload, schema, allot_tag);
}

tuple_rw_fixed tuple_rw_fixed::clone(const tuple_ro_managed &src,
                                     const defaults &) {
  return clone(src, scale2items[defaults::scale], scale2more[defaults::scale],
               defaults::schema.get(), defaults::allot_tag);
}
tuple_rw_fixed tuple_rw_fixed::clone(const tuple_ro_managed &src,
                                     const initiation_scale scale) {
  return clone(src, scale2items[scale], scale2more[scale],
               defaults::schema.get(), defaults::allot_tag);
}
tuple_rw_fixed tuple_rw_fixed::clone(const tuple_ro_managed &src,
                                     const initiation_scale scale,
                                     const fptu::schema *schema,
                                     const hippeus::buffer_tag &allot_tag) {
  return clone(src, scale2items[scale], scale2more[scale], schema, allot_tag);
}
tuple_rw_fixed tuple_rw_fixed::clone(const tuple_ro_managed &src,
                                     std::size_t more_items,
                                     std::size_t more_payload,
                                     const fptu::schema *schema,
                                     const hippeus::buffer_tag &allot_tag) {
  if (unlikely(!src))
    return tuple_rw_fixed(more_items, more_payload, schema, allot_tag);

  const details::tuple_rw *basis = src.peek_basis();
  if (basis && (basis->schema_ == schema || !schema)) {
    /* Если ro-кортеж основывает на буфере сделанном для rw-формы
     * и внутри этой формы была задана схема совпадающая с запрошенной,
     * либо если сейчас НЕ требуется какая-либо схема, то создаем копию без
     * проверки данных (в том числе без проверки на соответствие схеме). */
    const fptu::details::audit_holes_info holes_info = {basis->junk_.count,
                                                        basis->junk_.volume};
    return tuple_rw_fixed(holes_info, src.take_weak(), more_items, more_payload,
                          basis->schema_, allot_tag);
  }

  /* Если снизу нет буфера, либо схема отличается, то делаем копию с полной
   * проверкой данных (в том числе на соответствие схеме). */
  return clone(src.take_weak(), more_items, more_payload, schema, allot_tag);
}

tuple_rw_fixed::tuple_rw_fixed(tuple_ro_managed &&src) : pimpl_(nullptr) {
  if (unlikely(!src)) {
    /* Источник пуст (т.е. совсем нет кортежа, схемы и т.д.).
     * Стоит подумать, возможно вместо вброса исключения лучше создать
     * минимальный кортеж, или вынести это поведение в опции fptu::defaults. */
    throw_tuple_hollow();
  }

  const details::tuple_rw *basis = src.peek_basis();
  if (basis) {
    /* Снизу у tuple_ro_managed есть готовая R/W-форма */
    if (likely(src.hb_->is_alterable())) {
      /* Данные в монопольном использовании, просто перемещаем их */
      pimpl_ = const_cast<details::tuple_rw *>(basis);
      src.pimpl_ = nullptr;
      src.hb_ = nullptr;
      return;
    }
  } else if (likely(src.hb_->is_alterable()) &&
             erthink::constexpr_pointer_cast<const char *>(src.pimpl_->data()) -
                     erthink::constexpr_pointer_cast<const char *>(
                         src.hb_->data()) >=
                 ptrdiff_t(
                     offsetof(details::tuple_rw, reserve_for_RO_header))) {
    /* Буфер в монопольном использовании и есть место для управляющих данных
     * details::tuple_rw.
     * TODO: Конструируем кортеж по-месту без копирования и перемещаем буфер
     * из src в созданный tuple_rw_fixed. */
  }

  /* Копируем и отстыковываем данные. */
  tuple_rw_fixed duplicate(
      src.data(), src.size(), defaults::scale,
      /* подключаем схему, есть есть */ basis ? basis->schema() : nullptr,
      src.hb_->host);
  src.purge();
  std::swap(pimpl_, duplicate.pimpl_);
}

tuple_rw_fixed &tuple_rw_fixed::operator=(tuple_rw_fixed &&src) {
  if (likely(pimpl_ != src.pimpl_)) {
    purge();
    pimpl_ = src.pimpl_;
    src.pimpl_ = nullptr;
  }
  return *this;
}

tuple_rw_fixed &tuple_rw_fixed::operator=(tuple_ro_managed &&src) {
  return *this = tuple_rw_fixed(std::move(src));
}

tuple_rw_fixed tuple_rw_fixed::clone(const tuple_rw_fixed &src,
                                     const hippeus::buffer_tag &allot_tag) {
  if (unlikely(!src)) {
    /* Вбрасываем исключение вместо создания пустого кортежа без схемы,
     * так как последствия отсутствия схемы могут быть неожиданными
     * для пользователя. */
    throw_tuple_hollow();
  }

  return tuple_rw_fixed(src.pimpl_->create_copy(
      src.head_space(), src.tail_space_bytes(), allot_tag));
}
tuple_rw_fixed tuple_rw_fixed::clone(const tuple_rw_fixed &src,
                                     const initiation_scale scale,
                                     const hippeus::buffer_tag &allot_tag) {
  return clone(src, scale2items[scale], scale2more[scale], allot_tag);
}
tuple_rw_fixed tuple_rw_fixed::clone(const tuple_rw_fixed &src,
                                     std::size_t more_items,
                                     std::size_t more_payload,
                                     const hippeus::buffer_tag &allot_tag) {
  if (unlikely(!src)) {
    /* Вбрасываем исключение вместо создания пустого кортежа без схемы,
     * так как последствия отсутствия схемы могут быть неожиданными
     * для пользователя. */
    throw_tuple_hollow();
  }

  return tuple_rw_fixed(
      src.pimpl_->create_copy(more_items, more_payload, allot_tag));
}

//------------------------------------------------------------------------------

inline void tuple_ro_managed::check_buffer() const {
  if (unlikely((pimpl_ != nullptr) != (hb_ != nullptr)))
    throw_buffer_mismatch();
  if (hb_) {
    if (unlikely(static_cast<const uint8_t *>(pimpl_->data()) < hb_->begin() ||
                 static_cast<const uint8_t *>(pimpl_->data()) + pimpl_->size() >
                     hb_->end()))
      throw_buffer_mismatch();
  }
}

tuple_ro_managed::tuple_ro_managed(const impl *ro,
                                   const hippeus::buffer *buffer)
    : pimpl_(ro), hb_(buffer) {
  check_buffer();
  hb_->add_reference(/* accepts nullptr */);
}

tuple_ro_managed::tuple_ro_managed(const tuple_rw_fixed &src,
                                   const hippeus::buffer_tag &allot_tag)
    : tuple_ro_managed(src.take_managed_clone_asis(false, allot_tag)) {}

tuple_ro_managed::tuple_ro_managed(tuple_rw_fixed &&src)
    : pimpl_(src.take_weak_asis().pimpl_), hb_(src.get_buffer()) {
  check_buffer();
  src.pimpl_ = nullptr;
}

tuple_ro_managed &tuple_ro_managed::operator=(tuple_rw_fixed &&src) {
  return *this = tuple_ro_managed(std::move(src));
}

tuple_ro_managed &tuple_ro_managed::operator=(const tuple_ro_managed &src) {
  src.check_buffer();
  if (likely(hb_ != src.hb_)) {
    purge();
    hb_ = src.hb_->add_reference();
  }
  pimpl_ = src.pimpl_;
  return *this;
}

tuple_ro_managed &tuple_ro_managed::operator=(tuple_ro_managed &&src) {
  src.check_buffer();
  purge();
  hb_ = src.hb_;
  pimpl_ = src.pimpl_;
  src.pimpl_ = nullptr;
  src.hb_ = nullptr;
  return *this;
}

tuple_ro_managed::tuple_ro_managed(tuple_ro_managed &&src)
    : pimpl_(src.pimpl_), hb_(src.hb_) {
  check_buffer();
  src.pimpl_ = nullptr;
  src.hb_ = nullptr;
}

tuple_ro_managed tuple_ro_managed::clone(const tuple_ro_managed &src,
                                         const fptu::schema *schema,
                                         const hippeus::buffer_tag &allot_tag) {
  if (unlikely(!src)) {
    /* если источник пуст */
    if (schema)
      throw_tuple_hollow();
    return tuple_ro_managed();
  }

  if (src.hb_->host ==
          /* если буфер источника совместим с запрошенным */ allot_tag &&
      (!schema ||
       src.peek_schema() == schema /* и если схема тоже совпадает */))
    /* возвращаем ссылку на существующий буфер */
    return tuple_ro_managed(src.pimpl_, src.hb_);

  /* иначе делаем полную копию с полной проверкой на соответствие схеме */
  return clone(src.take_weak(), schema, allot_tag);
}

__pure_function const details::tuple_rw *tuple_ro_managed::peek_basis() const
    noexcept {
  if (unlikely(!pimpl_))
    return nullptr;

#if !defined(NDEBUG) && !defined(__COVERITY__)
  check_buffer(/* paranoia */);
#endif
  const details::tuple_rw *maybe =
      erthink::constexpr_pointer_cast<const details::tuple_rw *>(hb_->data());
  if (&maybe->reserve_for_RO_header >
      erthink::constexpr_pointer_cast<const details::unit_t *>(pimpl_))
    return nullptr;
  if (maybe->buffer_offset_ != maybe->get_buffer_offset(hb_))
    return nullptr;
  if (&maybe->area_[maybe->head_] - 1 !=
      erthink::constexpr_pointer_cast<const details::unit_t *>(pimpl_))
    return nullptr;

  assert(maybe->begin_index() == pimpl_->begin_index() &&
         maybe->end_index() == pimpl_->end_index());
  assert(maybe->begin_data_units() == pimpl_->begin_data_units() &&
         maybe->end_data_units() == pimpl_->end_data_units());
  return maybe;
}

tuple_ro_managed::tuple_ro_managed(const void *source_ptr,
                                   std::size_t source_bytes,
                                   const fptu::schema *schema,
                                   const hippeus::buffer_tag &allot_tag)
    /* Конструктор из внешнего источника с выделением управляемого буфера
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
     *  - Если же обеспечивать полную совместимость с details::tuple_rw, то
     *    возникает вопрос о необходимости класса tuple_ro_managed как такового,
     *    ибо проще использовать details::tuple_rw, скрыв изменяющие методы.
     *
     * Итоговое решение:
     *  - При создании tuple_ro_managed с выделением буфера и копированием
     *    в него данных производится создание полноценного экземпляра
     *    details::tuple_rw, с последующим перемещением в экземпляр
     *    tuple_ro_managed.
     *  - При создании tuple_ro_managed из существующего буфера (например при
     *    чтении вложенного кортежа) никаких дополнительных действий
     *    не предпринимается.
     *  - Перемещающие конструкторы R/W-форм из tuple_ro_managed должны получать
     *    указатель на схему, самостоятельно определять способ, которым был
     *    создан tuple_ro_managed и действовать соответствующим образом:
     *     1) Если буфер НЕ в монопольном использовании, то конструктор должен
     *        сделать копию данных вне зависимости от способа создания.
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
    : tuple_ro_managed(tuple_rw_fixed(source_ptr, source_bytes,
                                      /* more_items */ 0, /* more_payload */ 0,
                                      schema, allot_tag)) {}

tuple_ro_managed tuple_ro_managed::clone(const tuple_ro_weak &src,
                                         const fptu::schema *schema,
                                         const hippeus::buffer_tag &allot_tag,
                                         validation_mode validation) {
  /* Клонирование из уже проверенной R/O-формы в собственный буфер.
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
   *   - Если схема не задана (по-умолчанию), то опорный экземпляр
   *     details::tuple_rw создается без схемы и без проверки данных.
   *   - Иначе, если схема задана, то выполняется полная (возможно повторная)
   *     проверка данных (в том числе на соответствие схеме), а опорный
   *     экземпляр details::tuple_rw создается со схемой. */

  if (unlikely(!src)) {
    /* если источник пуст */
    if (schema)
      throw_tuple_hollow();
    return tuple_ro_managed();
  }

  const bool wanna_validation = apply_validation_mode(
      validation,
      /* если схема не задана, то считаем что необходимые проверки были
         сделаны при создании tuple_ro_weak, иначе выполняем повторную
         проверку для исключения несоответствия схеме */
      schema == nullptr);
  if (wanna_validation)
    return tuple_ro_managed(tuple_rw_fixed(
        src.data(), src.size(),
        /* more_items */ 0, /* more_payload */ 0, schema, allot_tag));

  const details::audit_holes_info &holes_info = {0, 0};
  return tuple_ro_managed(tuple_rw_fixed(
      holes_info, src,
      /* more_items */ 0, /* more_payload */ 0, schema, allot_tag));
}

} // namespace fptu
