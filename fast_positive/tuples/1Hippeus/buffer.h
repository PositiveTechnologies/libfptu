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
 *
 * ***************************************************************************
 *
 * Imported (with simplification) from 1Hippeus project.
 * Copyright (c) 2006-2013 Leonid Yuriev <leo@yuriev.ru>.
 */

#pragma once
#include "fast_positive/erthink/erthink_casting.h++"
#include "fast_positive/tuples/1Hippeus/actor.h"
#include "fast_positive/tuples/1Hippeus/hipagut.h"
#include "fast_positive/tuples/1Hippeus/utils.h"
#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/details/tagged_pointer.h"
#include "fast_positive/tuples/details/uio.h"

#include "fast_positive/tuples/details/warnings_push_system.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef __cplusplus
#include <climits>
#include <memory>
#else
#include <limits.h>
#endif

#include "fast_positive/tuples/details/warnings_pop.h"

#include "fast_positive/tuples/details/warnings_push_pt.h"

/* Буфера выделенные под данные или структуры могут пересекать границы процессов
 * и освобождаться вне контекста обмена сообщениями. Например, часть данных из
 * пакета принятого с сетевого интерфейса может быть помещена в разделяемое
 * хранилище. Соответственно, буфер выделенный под пакет, может быть освобожден
 * позже при обновлении данных или очистке хранилища.
 *
 * Поэтому:
 * - аллокаторы отделены от точек соединения и транспортных провайдеров.
 * - буфера привязываются к аллокатору только посредством тега (идентификатора
 *   аллокатора).
 * - теги аллокаторов глобально уникальны и назначаются binder-ом при
 *   регистрации аллокатора.
 * - если точка соединения или транспортный провайдер требуют своего аллокатора,
 *   то должны предоставить его и обеспечить работу до освобождения всех
 *   буферов.
 *
 * Для дефицитных буферов следует выставлять флаг HIPPEUS_SCARCE. Соответственно
 * такие буфера:
 * - не должны использоваться для хранения данных,
 *   а только для организации очередей и обмена данными.
 * - если данные требуются больше чем на время цикла обработки сообщения,
 *   то их следует освобождать или клонировать.
 */

//-----------------------------------------------------------------------------

/* Omitted typedefs.
 * These types not required for libftpu for now,
 * therefore ones is omitted for simplicity. */
typedef struct hippeus_handle_C hippeus_handle_t;
typedef struct hippeus_object_C hippeus_object_t;

//-----------------------------------------------------------------------------

/* LY: Флажки в заголовках буферов. */
enum hippeus_buffer_flags {
  HIPPEUS_READONLY = 1 /*!< Буфер с данными только для чтения */,
  HIPPEUS_LOCALWEAK =
      2 /*!< Локальный псевдо-разделяемый буфер, управляемый локальным
           аллокатором (внутри адресного пространства текущего процесса), либо
           классом С++ с виртуальным деструктором */
  ,
  HIPPEUS_SCARCE = 4 /*!< Дефицитный буфер, следует вернуть как можно скорее */
  ,
  HIPPEUS_FLAG_RESERVED = 8,
  HIPPEUS_FLAG_BITS = 4
};
DEFINE_ENUM_FLAG_OPERATORS(hippeus_buffer_flags)

/* typedefs for C */
typedef struct hippeus_buffer_C hippeus_buffer_t;
typedef struct hippeus_allot_C hippeus_allot_t;
typedef struct hippeus_buffer_tag_C hippeus_buffer_tag_t;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(                                                               \
    disable : 4324) /* structure was padded due to alignment specifier */
#endif

struct FPTU_API_TYPE
#if defined(__GNUC__) /* GCC всех версий болеет, встречая alignas() рядом с \
                         __attribute__((__visibility__("default"))) */
    __attribute__((__aligned__(1u << HIPPEUS_FLAG_BITS)))
#else
    alignas(1u << HIPPEUS_FLAG_BITS)
#endif
    hippeus_allot_C {
#ifdef HIPPEUS
  hippeus_object_t obj;
  hippeus_handle_t handle;
#endif /* HIPPEUS */
  unsigned default_chunk;
  unsigned flags;

  hippeus_buffer_t *(*borrow)(hippeus_allot_t *allot, std::size_t wanna_size,
                              bool leastwise, hippeus_actor_t actor);
  void (*repay)(hippeus_allot_t *allot, hippeus_buffer_t *,
                hippeus_actor_t actor);
  __must_check_result bool (*validate_)(const hippeus_allot_t *allot,
                                        bool probe_only, bool deep_checking);
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

//-----------------------------------------------------------------------------

/* Тэг для привязки буфера к аллокатору, а также пометки самих буферов (см.
 * определения выше). Все флажки и значения хранятся как битовые поля в одном
 * opacity и доступны только через методы. */
struct FPTU_API_TYPE hippeus_buffer_tag_C {
  union FPTU_API_TYPE casting {
    uintptr_t uint;
    void *ptr;
    const void *const_ptr;
#ifdef __cplusplus
    cxx11_constexpr casting(const casting &) cxx11_noexcept = default;
    cxx11_constexpr casting(const void *ptr) cxx11_noexcept : const_ptr(ptr) {}
    cxx11_constexpr casting(uintptr_t uint) cxx11_noexcept : uint(uint) {}
#endif /* __cplusplus */
  } opacity_;

#ifdef __cplusplus
  friend cxx11_constexpr bool
  operator==(const hippeus_buffer_tag_C &a,
             const hippeus_buffer_tag_C &b) cxx11_noexcept {
    return a.opacity_.uint == b.opacity_.uint;
  }
  friend cxx11_constexpr bool
  operator!=(const hippeus_buffer_tag_C &a,
             const hippeus_buffer_tag_C &b) cxx11_noexcept {
    return a.opacity_.uint != b.opacity_.uint;
  }

protected:
  cxx11_constexpr hippeus_buffer_tag_C(uintptr_t uint) cxx11_noexcept
      : opacity_(uint) {}
  cxx11_constexpr
  hippeus_buffer_tag_C(const hippeus_buffer_tag_C &) cxx11_noexcept = default;
#endif /* __cplusplus */
};

#ifdef __cplusplus
namespace hippeus {

struct FPTU_API_TYPE buffer_tag : public hippeus_buffer_tag_C {
  /* LY: Флаги для управления буфером (readonly, local и т.д.).
   * При некоторых комбинациях (когда установлен флаг HIPPEUS_LOCALWEAK)
   * остальные поля теряют смысл, а общее значение opacity соответствует
   * прямому локальному указателю на интерфейс аллокатора. */
  enum {
    HIPPEUS_TAG_FLAGS_BITS = HIPPEUS_FLAG_BITS,
    HIPPEUS_TAG_ALLOT_BITS = (sizeof(uintptr_t) > 4) ? 28 : 12,
    HIPPEUS_TAG_DEPOT_BITS = (sizeof(uintptr_t) > 4) ? 16 : 8,
    HIPPEUS_TAG_CRATE_BITS = (sizeof(uintptr_t) > 4) ? 16 : 8,
    HIPPEUS_TAG_BITS = HIPPEUS_TAG_FLAGS_BITS + HIPPEUS_TAG_ALLOT_BITS +
                       HIPPEUS_TAG_DEPOT_BITS + HIPPEUS_TAG_CRATE_BITS,

    HIPPEUS_TAG_ALLOT_SHIFT = HIPPEUS_TAG_FLAGS_BITS,
    HIPPEUS_TAG_DEPOT_SHIFT = HIPPEUS_TAG_ALLOT_SHIFT + HIPPEUS_TAG_ALLOT_BITS,
    HIPPEUS_TAG_CRATE_SHIFT = HIPPEUS_TAG_DEPOT_SHIFT + HIPPEUS_TAG_DEPOT_BITS,
    HIPPEUS_TAG_FLAGS_MASK = (1u << HIPPEUS_TAG_FLAGS_BITS) - 1,
    HIPPEUS_TAG_ALLOT_MASK = (1u << HIPPEUS_TAG_ALLOT_BITS) - 1,
    HIPPEUS_TAG_DEPOT_MASK = (1u << HIPPEUS_TAG_DEPOT_BITS) - 1,
    HIPPEUS_TAG_CRATE_MASK = (1u << HIPPEUS_TAG_CRATE_BITS) - 1,
    HIPPEUS_TAG_LOCALPTR_THRESHOLD = HIPPEUS_TAG_FLAGS_MASK + 1
  };

  using base = hippeus_buffer_tag_C;

  static cxx14_constexpr uintptr_t p2u(const void *ptr) cxx11_noexcept {
    return erthink::bit_cast<uintptr_t>(ptr);
  }

  static cxx14_constexpr void *u2p(uintptr_t uint) cxx11_noexcept {
    return erthink::bit_cast<void *>(uint);
  }

  cxx11_constexpr buffer_tag() cxx11_noexcept : base(0) {
    static_assert(sizeof(buffer_tag) == sizeof(hippeus_buffer_tag_C), "WTF?");
    static_assert(sizeof(uintptr_t) * CHAR_BIT == HIPPEUS_TAG_BITS, "WTF?");
  }

  cxx11_constexpr buffer_tag(const buffer_tag &) cxx11_noexcept = default;
  cxx14_constexpr buffer_tag &
  operator=(const buffer_tag &) cxx11_noexcept = default;

  cxx14_constexpr buffer_tag(const hippeus_allot_C *local_allot, bool readonly)
      : base(p2u(local_allot) | (readonly ? HIPPEUS_LOCALWEAK | HIPPEUS_READONLY
                                          : HIPPEUS_LOCALWEAK)) {
    CONSTEXPR_ASSERT((erthink::bit_cast<uintptr_t>(local_allot) &
                      HIPPEUS_TAG_FLAGS_MASK) == 0);
  }

  cxx11_constexpr buffer_tag(unsigned allot_id, unsigned depot, unsigned crate,
                             hippeus_buffer_flags flags) cxx11_noexcept
      : base(uintptr_t(flags) | uintptr_t(allot_id) << HIPPEUS_TAG_ALLOT_SHIFT |
             uintptr_t(depot) << HIPPEUS_TAG_DEPOT_SHIFT |
             uintptr_t(crate) << HIPPEUS_TAG_CRATE_SHIFT) {
    CONSTEXPR_ASSERT((flags & HIPPEUS_LOCALWEAK) == 0);
    CONSTEXPR_ASSERT(unsigned(flags) <= unsigned(HIPPEUS_TAG_FLAGS_MASK) &&
                     allot_id <= HIPPEUS_TAG_ALLOT_MASK &&
                     depot <= HIPPEUS_TAG_DEPOT_MASK &&
                     crate <= HIPPEUS_TAG_CRATE_MASK);
    CONSTEXPR_ASSERT(this->flags() == flags && this->allot_id() == allot_id &&
                     this->allot_depot() == depot &&
                     this->allot_crate() == crate);
  }

  cxx11_constexpr hippeus_buffer_flags flags() const cxx11_noexcept {
    return hippeus_buffer_flags(opacity_.uint & HIPPEUS_TAG_FLAGS_MASK);
  }

  cxx11_constexpr bool
  flags_test(hippeus_buffer_flags mask) const cxx11_noexcept {
    CONSTEXPR_ASSERT(unsigned(mask) <= unsigned(HIPPEUS_TAG_FLAGS_MASK));
    return (uintptr_t(mask) & opacity_.uint) != 0;
  }

  /* LY: Идентификатор аллокатора и региона разделяемой памяти.
   * Гарантируется уникальность идентификаторов в пределах кооперации (инстанции
   * 1Hippeus).
   *
   * Технически, это хендл объекта-аллокатора, укороченный из соображений
   * экономии, который позволяет через баиндер получить доступ к телу и
   * интерфейсу аллокатора. */
  cxx11_constexpr unsigned allot_id() const cxx11_noexcept {
    CONSTEXPR_ASSERT(!flags_test(HIPPEUS_LOCALWEAK));
    return (opacity_.uint >> HIPPEUS_TAG_ALLOT_SHIFT) & HIPPEUS_TAG_ALLOT_MASK;
  }

  /* ВАЖНО: Содержание и интерпретация остальных "полей" depot & basket
   * определяется аллокатором. Функции ниже соответствуют логике стандартного
   * аллокатора, который таким образом хранит
   * необходимые индексы - это позволет очень быстро обработать освобождаемый
   * буфер. */

  /* Индекс пула в регионе.
   * Номера пулов уникальны в пределах региона.
   * Точная семантика может быть другой и определяется реализацией аллокатора.
   */
  cxx11_constexpr unsigned allot_depot() const cxx11_noexcept {
    CONSTEXPR_ASSERT(!flags_test(HIPPEUS_LOCALWEAK));
    return (opacity_.uint >> HIPPEUS_TAG_DEPOT_SHIFT) & HIPPEUS_TAG_DEPOT_MASK;
  }

  /* Индекс/номер корзины (набора страниц) в пуле, из которой выделен буфер.
   * Номера корзин уникальны в пределах пула.
   * Точная семантика может быть другой и определяется реализацией аллокатора.
   */
  cxx11_constexpr unsigned allot_crate() const cxx11_noexcept {
    CONSTEXPR_ASSERT(!flags_test(HIPPEUS_LOCALWEAK));
    return (opacity_.uint >> HIPPEUS_TAG_CRATE_SHIFT) & HIPPEUS_TAG_CRATE_MASK;
  }

  /* Прямой указатель на локальный аллокатор. */
  cxx14_constexpr hippeus_allot_C *local_allot() const cxx11_noexcept {
    CONSTEXPR_ASSERT(flags_test(HIPPEUS_LOCALWEAK));
    return static_cast<hippeus_allot_C *>(
        u2p(opacity_.uint & ~uintptr_t(HIPPEUS_TAG_FLAGS_MASK)));
  }

  cxx11_constexpr uintptr_t raw() const cxx11_noexcept { return opacity_.uint; }

  cxx11_constexpr operator bool() const cxx11_noexcept {
    return opacity_.uint != 0;
  }
};

} // namespace hippeus
#endif /* __cplusplus */

//-----------------------------------------------------------------------------

/* LY: Заголовок буфера.
 * Содержит относительный указатель на начало, размер доступного места, счетчик
 * ссылок и привязку к аллокатору. Буфер определяет только доступное место,
 * полезные данные могут занимать его произвольную часть.
 *
 * Указатель на данные являет компромиссом, который позволят привести к одной
 * форме различные варианты организации буферов. В частности, данные могут
 * следовать непосредственно за заголовком буфера, либо располагаться в другом
 * месте. */
struct FPTU_API_TYPE hippeus_buffer_C {
  HIPAGUT_DECLARE(guard_head);

  // Относительный указатель на место размещения данных,
  // для сплошных/непосредственных буферов указывает на _inplace.
  ptrdiff_t _data_offset;

  /* LY: координаты полезной нагрузки в отправленном сообщении для простейшего
   * случая, когда всё сообщение состоит из одного монопольно используемого
   * буфера. */
  union {
    struct {
      uint32_t offset;
      uint32_t length;
    };
    uint64_t offset_length;
  } simple_msg;

  // Размер места под данные, не считая заголовков и гардов.
  uint32_t space;

  // Счётчик ссылок, если меньше 2, то изменять его может только
  // единственный текущий владелец.
  volatile mutable int32_t ref_counter;

  // Идентификатор и служебные данные аллокатора, который управляет буфером.
#ifdef __cplusplus
  hippeus::buffer_tag host;
#else
  hippeus_buffer_tag_t host;
#endif /* __cplusplus */

  HIPAGUT_DECLARE(guard_under);

  // Место под данные для сплошных/непосредственных буферов.
  uint8_t _inplace[1];
};

__extern_C bool hippeus_buffer_enforce_deep_checking;

//-----------------------------------------------------------------------------
#ifdef __cplusplus

namespace hippeus {

/*! \brief Основной интерфейсный класс для всех видов буферов.
 * Не должен использоваться для создания буферов. Для этого предназначены
 * производные классы и аллокаторы. */
class FPTU_API_TYPE buffer : public hippeus_buffer_C {
  /*! Class is closed and noncopyable. */
  buffer(const buffer &) = delete;
  buffer &operator=(buffer const &) = delete;

  static void init(buffer *, buffer_tag host, void *payload,
                   ptrdiff_t payload_bytes);

  buffer(const buffer_tag host, void *payload, ptrdiff_t payload_bytes) {
    static_assert(sizeof(buffer) == sizeof(hippeus_buffer_C), "WTF?");
    init(this, host, payload, payload_bytes);
  }

  ~buffer() cxx11_noexcept = default;

  friend class buffer_solid;
  friend class buffer_indirect;
  friend class buffer_weak;

  static const buffer *
  add_reference(/* accepts nullptr */ const buffer *self) cxx11_noexcept;
  static void detach(/* accepts nullptr */ const buffer *self) cxx11_noexcept;

public:
  /* Основная функция для выделения буферов. Теоретически тут также стоит
   * удалить всевозможные варианты new и delete, но целесообразность вызывает
   * сомнения:
   *  - приватности конструкторов достаточно для предотвращения ошибочных
   *    действий;
   *  - operator new() может потребоваться для наследников buffer_weak.
   */
  static buffer *borrow(buffer_tag tag, std::size_t wanna, bool leastwise);
  void operator delete(void *ptr) { detach(static_cast<buffer *>(ptr)); }
  void *operator new[](std::size_t) = delete;
  void operator delete[](void *ptr) = delete;

  /* non-allocating placement (de)allocation functions
   * Компиляторы на основе EDG-фроненда могут болеть без их явного определения
   * при использовании placement new */
  void *operator new(std::size_t bytes, void *ptr) noexcept {
    assert(bytes >= sizeof(buffer));
    (void)bytes;
    return ptr;
  }
  void operator delete(void *ptr, void *place) noexcept {
    assert(ptr == place);
    (void)ptr;
    (void)place;
  }

  const buffer *add_reference(/* accepts nullptr */) const cxx11_noexcept {
    return add_reference(this);
  }
  buffer *add_reference(/* accepts nullptr */) cxx11_noexcept {
    return const_cast<buffer *>(add_reference(this));
  }

  void detach() const cxx11_noexcept {
    /* follows naming convention from
     * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0468r0.html */
    detach(this);
  }

  bool is_readonly() const cxx11_noexcept {
    return (host.opacity_.uint & uintptr_t(HIPPEUS_READONLY)) != 0;
  }
  bool is_localweak() const cxx11_noexcept {
    return (host.opacity_.uint & uintptr_t(HIPPEUS_LOCALWEAK)) != 0;
  }
  bool is_warpable() const cxx11_noexcept {
    return (host.opacity_.uint &
            uintptr_t(HIPPEUS_LOCALWEAK | HIPPEUS_READONLY)) == 0;
  }
  bool is_alterable() const cxx11_noexcept {
    return ref_counter == 1 && !is_readonly();
  }

  std::size_t size() const cxx11_noexcept { return space; }
  const void *data() const cxx11_noexcept {
    return static_cast<const void *>(begin());
  }
  void *data() cxx11_noexcept { return static_cast<void *>(begin()); }

  uint8_t *begin() cxx11_noexcept {
    CONSTEXPR_ASSERT(ensure());
    CONSTEXPR_ASSERT(_data_offset);
    return erthink::constexpr_pointer_cast<uint8_t *>(this) + _data_offset;
  }
  uint8_t *end() cxx11_noexcept { return begin() + size(); }

  const uint8_t *cbegin() const cxx11_noexcept {
    CONSTEXPR_ASSERT(ensure());
    CONSTEXPR_ASSERT(_data_offset);
    return erthink::constexpr_pointer_cast<const uint8_t *>(this) +
           _data_offset;
  }
  const uint8_t *cend() const cxx11_noexcept { return begin() + size(); }

  const uint8_t *begin() const cxx11_noexcept { return cbegin(); }
  const uint8_t *end() const cxx11_noexcept { return cend(); }

  bool is_solid() const cxx11_noexcept {
    return _data_offset == offsetof(buffer, _inplace);
  }

  void pollute(uintptr_t xormask = 0) {
    assert(!is_readonly());
    hippeus::pollute(begin(), size(), xormask);
  }

  void bzero() {
    assert(!is_readonly());
    ::memset(begin(), 0, size());
  }

  bool __must_check_result check(bool deep_checking = false) const;
  bool __must_check_result
  check_expect_invalid(bool deep_checking = false) const;
  bool ensure(bool deep_checking = false) const;
};

/* glue for boost::intrusive_ptr */
static __maybe_unused __always_inline void
intrusive_ptr_add_ref(buffer *ptr) cxx11_noexcept {
  ptr->add_reference();
}
static __maybe_unused __always_inline void intrusive_ptr_release(buffer *ptr) {
  ptr->detach();
}

//------------------------------------------------------------------------------

/*! \brief Lineal chunk of memory, where payload placed immediatly following the
   head. This is the primary class of shareable buffers.
*/
/* LY: Сплошной буфер для данных, извествен также как непосредственый или
 * direct. Данные следуют сразу за заголовком буфера и вся эта память выделяется
 * аллокатором одновременно. Это основной "родной" тип буферов для размещения в
 * разделямой памяти.
 *
 * Наследование не допускается, иначе возможны проблемы с разделяемой памятью.
 * Такие буфера должны создаваться только аллокаторами, конструктор сугубо
 * утилитарный. */
class FPTU_API_TYPE buffer_solid final : public buffer {
public:
  static cxx11_constexpr std::size_t space_overhead() cxx11_noexcept {
    return sizeof(buffer) + HIPAGUT_SPACE;
  }
  buffer_solid(buffer_tag host, std::size_t gross_bytes)
      : buffer(host, _inplace, gross_bytes - space_overhead()) {
    assert(!is_localweak() ||
           this->host.raw() >
               uintptr_t(hippeus::buffer_tag::HIPPEUS_TAG_LOCALPTR_THRESHOLD));
  }
};

/* LY: Косвенный (indirect) буфер для данных.
 * Данные располагаются отдельно от заголовка, но с точки зрения 1Hippeus
 * это одна сущность с одним счетчиком ссылок в заголовке. Аллокатор отвечает
 * для выделение и освобождение участков памяти под данные и заголовок.
 *
 * Основное назначение таких буферов в интеграции с внешними подсистемами и
 * аллокаторами. При этом реализуется легковесный аллокатор, который выделяет и
 * освобождает заголовки в своем пуле памяти, а под данные использует "внешние"
 * буфера от интегрируемой подсистемы.
 *
 * Наследование не допускается, иначе возможны проблемы с разделяемой памятью.
 * Такие буфера должны создаваться только аллокаторами, конструктор сугубо
 * утилитарный.
 *
 * В отличии от "хлипкого" буфера (см. далее) косвенный буфер может размещаться
 * в разделяемой памяти и переходить границы адресных пространств процессов. */
class FPTU_API_TYPE buffer_indirect final : public buffer {
public:
  buffer_indirect(const buffer_tag host, void *payload,
                  std::size_t payload_bytes)
      : buffer(host, payload, payload_bytes) {
    assert(!is_localweak());
    assert(payload != _inplace);
  }
};

/*! \brief Head separated off from the payload, which may by from external
   source. Its allow flexibility to bind data from mapped file or other object.
   But on other hand this disallow pass over shared memory.
*/
/* LY: Так называемый "хлипкий" буфер.
 * Основное назначение таких буферов в интеграции с инфраструктурой С++
 * в рамках одного процесса. Как класс С++ допускает порождение
 * производных классов и имеет виртуальный деструктор.
 *
 * Не допускается размещение в разделяемой памяти и пересечение границы
 * процесса. Вызов виртуального деструктора заложено в логику кода управления
 * буферами и происходит автоматически при наличии флажка HIPPEUS_WEAK. */
class FPTU_API_TYPE buffer_weak : public buffer {
protected:
  buffer_weak(const buffer_tag host, void *payload, std::size_t payload_bytes)
      : buffer(host, payload, payload_bytes) {
    assert(payload != _inplace);
    assert(is_localweak());
  }

public:
  virtual ~buffer_weak();
};

struct FPTU_API_TYPE buffer_deleter {
  void operator()(buffer *ditto) const { ditto->detach(); }
};

using buffer_ptr = std::unique_ptr<buffer, buffer_deleter>;

inline buffer_ptr clone(const buffer_ptr &ditto) {
  buffer *body = ditto.get();
  body->add_reference();
  return buffer_ptr(body);
}

} // namespace hippeus
#endif /* __cplusplus */

extern FPTU_API hippeus_allot_t hippeus_allot_stdcxx;

#ifdef __cplusplus
namespace hippeus {
static inline buffer_tag default_allot_tag() {
  return hippeus::buffer_tag(&hippeus_allot_stdcxx, false);
}
} // namespace hippeus
#endif /* __cplusplus */

#include "fast_positive/tuples/details/warnings_pop.h"
