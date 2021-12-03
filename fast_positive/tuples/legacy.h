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
#ifndef FAST_POSITIVE_TUPLES_H
#define FAST_POSITIVE_TUPLES_H

#ifdef _MSC_VER
#pragma warning(push)
#if _MSC_VER < 1900
#pragma warning(disable : 4350) /* behavior change: 'std::_Wrap_alloc... */
#endif
#if _MSC_VER > 1913
#pragma warning(disable : 5045) /* will insert Spectre mitigation... */
#endif
#pragma warning(disable : 4514) /* 'xyz': unreferenced inline function         \
                                   has been removed */
#pragma warning(disable : 4710) /* 'xyz': function not inlined */
#pragma warning(disable : 4711) /* function 'xyz' selected for                 \
                                   automatic inline expansion */
#pragma warning(disable : 4061) /* enumerator 'abc' in switch of enum 'xyz' is \
                                   not explicitly handled by a case label */
#pragma warning(disable : 4201) /* nonstandard extension used :                \
                                   nameless struct / union */
#pragma warning(disable : 4127) /* conditional expression is constant */
#pragma warning(disable : 4702) /* unreachable code */
#pragma warning(push, 1)
#ifndef _STL_WARNING_LEVEL
#define _STL_WARNING_LEVEL 3 /* Avoid warnings inside nasty MSVC STL code */
#endif
#pragma warning(disable : 4548) /* expression before comma has no effect;      \
                                   expected expression with side - effect */
#pragma warning(disable : 4530) /* C++ exception handler used, but unwind      \
                                   semantics are not enabled. Specify /EHsc */
#pragma warning(disable : 4577) /* 'noexcept' used with no exception handling  \
                                   mode specified; termination on exception    \
                                   is not guaranteed. Specify /EHsc */
#endif                          /* _MSC_VER (warnings) */
//------------------------------------------------------------------------------

#include <errno.h>  // for error codes
#include <float.h>  // for FLT_EVAL_METHOD and float_t
#include <limits.h> // for INT_MAX, etc
#include <math.h>   // for NaNs
#include <stdio.h>  // for FILE
#include <string.h> // for strlen()
#include <time.h>   // for struct timespec, struct timeval

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/* FIXME: обеспечить генерацию определений из C++ кода в процессе сборки */
#include "fast_positive/tuples.h"

//------------------------------------------------------------------------------

#define FPT_IS_POWER2(value) (((value) & ((value)-1)) == 0 && (value) > 0)
#define __FPT_FLOOR_MASK(type, value, mask) ((value) & ~(type)(mask))
#define __FPT_CEIL_MASK(type, value, mask)                                     \
  __FPT_FLOOR_MASK(type, (value) + (mask), mask)
#define FPT_ALIGN_FLOOR(value, align)                                          \
  __FPT_FLOOR_MASK(__typeof(value), value, (__typeof(value))(align)-1)
#define FPT_ALIGN_CEIL(value, align)                                           \
  __FPT_CEIL_MASK(__typeof(value), value, (__typeof(value))(align)-1)
#define FPT_IS_ALIGNED(ptr, align)                                             \
  ((((uintptr_t)(align)-1) & (uintptr_t)(ptr)) == 0)

//------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#else
typedef enum fptu_error fptu_error;
typedef union fptu_payload fptu_payload;
typedef union fptu_field fptu_field;
typedef union fptu_unit fptu_unit;
typedef union fptu_ro fptu_ro;
typedef struct fptu_rw fptu_rw;
typedef enum fptu_type fptu_type;
typedef enum fptu_filter fptu_filter;
typedef enum fptu_json_options fptu_json_options;
#endif /* __cplusplus */
typedef fptu_datetime_t fptu_time;

/* Коды ошибок.
 * Список будет пополнен, а описания уточнены. */
enum fptu_error {
  FPTU_SUCCESS = 0,
  FPTU_OK = FPTU_SUCCESS,
#if defined(_WIN32) || defined(_WIN64)
  FPTU_ENOFIELD = 0x00000650 /* ERROR_INVALID_FIELD */,
  FPTU_EINVAL = 0x00000057 /* ERROR_INVALID_PARAMETER */,
  FPTU_ENOSPACE = 0x00000540 /* ERROR_ALLOTTED_SPACE_EXCEEDED */,
#else
#ifdef ENOKEY
  FPTU_ENOFIELD = ENOKEY /* Required key not available */,
#else
  FPTU_ENOFIELD = ENOENT /* No such file or directory (POSIX) */,
#endif
  FPTU_EINVAL = EINVAL /* Invalid argument (POSIX) */,
  FPTU_ENOSPACE = ENOBUFS /* No buffer space available (POSIX)  */,
/* OVERFLOW - Value too large to be stored in data type (POSIX) */
#endif
  FPTU_ENOMEM = ENOMEM
};
FPTU_API void fptu_clear_error() /* thread safe */;
FPTU_API fptu_error fptu_last_error_code() /* thread safe */;
FPTU_API const char *fptu_last_error_msg() /* thread safe */;

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
union fptu_ro;

/* Изменяемая форма кортежа.
 * Является плоским буфером, в начале которого расположены служебные поля.
 *
 * Инициализируется функциями fptu_init(), fptu_alloc() и fptu_fetch(). */
struct fptu_rw;

/* Основные ограничения и константы. */
enum fptu_bits {
  // базовые лимиты и параметры
  fptu_typeid_bits =
      fptu::fundamentals::genus_bitness, // ширина типа в идентификаторе поля
  fptu_unit_size = fptu::fundamentals::unit_size, // размер одного юнита

  // производные константы и параметры
  // log2(fptu_unit_size)
  fptu_unit_shift = fptu::fundamentals::unit_shift,

  // максимальный суммарный размер сериализованного представления кортежа,
  // включая заголовок
  fptu_max_tuple_bytes = fptu::fundamentals::max_tuple_bytes_brutto,

  // максимальный тег-номер поля/колонки
  fptu_max_cols = fptu::details::tag_bits::max_ident,

  // максимальное кол-во значений полей (с учетом коллекций) в одном кортеже
  fptu_max_fields = fptu::fundamentals::max_fields,

  // максимальный размер поля/колонки
  fptu_max_field_bytes = fptu::fundamentals::max_field_bytes,

  // максимальный размер произвольной последовательности байт
  fptu_max_opaque_bytes = fptu_max_field_bytes - fptu_unit_size,

  // буфер достаточного размера для любого кортежа
  fptu_buffer_enough = fptu::fundamentals::buffer_enough,

  // предельный размер, превышение которого считается ошибкой
  fptu_buffer_limit = fptu::fundamentals::buffer_limit
};

/* Типы полей.
 *
 * Следует обратить внимание, что fptu_farray является флагом,
 * а значения начиная с fptu_flag_filter используются как маски для
 * поиска/фильтрации полей (и видимо будут выделены в отдельный enum). */
enum fptu_type : unsigned {
  fptu_null = ~0u,
  // fixed length, without ex-data (descriptor only)
  fptu_uint16 = fptu::details::make_tag(fptu::genus::u16, 0, true, true, false),
  fptu_bool =
      fptu::details::make_tag(fptu::genus::boolean, 0, true, true, false),

  // fixed length with ex-data (at least 4 byte after the pivot)
  fptu_int32 = fptu::details::make_tag(fptu::genus::i32, 0, true, true, false),
  fptu_uint32 = fptu::details::make_tag(fptu::genus::u32, 0, true, true, false),
  fptu_fp32 = fptu::details::make_tag(fptu::genus::f32, 0, true, true,
                                      false), // 32-bit float-point, e.g. float

  fptu_int64 = fptu::details::make_tag(fptu::genus::i64, 0, true, true, false),
  fptu_uint64 = fptu::details::make_tag(fptu::genus::u64, 0, true, true, false),
  fptu_fp64 = fptu::details::make_tag(fptu::genus::f64, 0, true, true,
                                      false), // 64-bit float-point, e.g. double
  fptu_datetime = fptu::details::make_tag(
      fptu::genus::t64, 0, true, true,
      false), // date-time as fixed-point, compatible with UTC

  fptu_96 = fptu::details::make_tag(fptu::genus::bin96, 0, true, true,
                                    false), // opaque 12-bytes.
  fptu_128 =
      fptu::details::make_tag(fptu::genus::bin128, 0, true, true,
                              false), // opaque 16-bytes (uuid, ipv6, etc).
  fptu_160 = fptu::details::make_tag(fptu::genus::bin160, 0, true, true,
                                     false), // opaque 20-bytes (sha1).
  fptu_256 = fptu::details::make_tag(fptu::genus::bin256, 0, true, true,
                                     false), // opaque 32-bytes (sha256).
  fptu_ip = fptu::details::make_tag(fptu::genus::ip, 0, true, true, false),
  fptu_mac = fptu::details::make_tag(fptu::genus::mac, 0, true, true, false),
  fptu_ipnet =
      fptu::details::make_tag(fptu::genus::ipnet, 0, true, true, false),

  // variable length, e.g. length and payload inside ex-data
  fptu_cstr = fptu::details::make_tag(fptu::genus::text, 0, true, true,
                                      false), // utf-8 string

  // with additional length
  fptu_opaque = fptu::details::make_tag(fptu::genus::varbin, 0, true, true,
                                        false), // opaque octet string
  fptu_nested = fptu::details::make_tag(fptu::genus::nested, 0, true, true,
                                        false), // nested tuple

  // aliases
  fptu_16 = fptu_uint16,
  fptu_32 = fptu_uint32,
  fptu_64 = fptu_uint64,
  fptu_uuid = fptu_128,
  fptu_md5 = fptu_128,
  fptu_sha1 = fptu_160,
  fptu_sha256 = fptu_256
};

enum fptu_filter {
  fptu_flag_not_filter = UINT32_C(1) << 31,
  fptu_any = fptu::details::mask_all_types,
  fptu_any_int = fptu::details::mask_integer,
  fptu_any_uint = fptu::details::mask_unsigned,
  fptu_any_fp = fptu::details::mask_float,
  fptu_any_number = fptu::details::mask_number
};
DEFINE_ENUM_FLAG_OPERATORS(fptu_filter)

static cxx11_constexpr fptu_filter fptu_filter_mask(fptu_type type) {
  return fptu_filter(UINT32_C(1) << fptu::details::tag2genus(type));
}

enum class fptu_type_or_filter : uint32_t {};
using fptu_tag_t = uint_fast16_t;

FPTU_API unsigned fptu_get_colnum(fptu_tag_t tag) cxx11_noexcept;
FPTU_API fptu_type fptu_get_type(fptu_tag_t tag) cxx11_noexcept;
FPTU_API uint_fast16_t fptu_make_tag(unsigned column,
                                     fptu_type type) cxx11_noexcept;
FPTU_API bool fptu_tag_is_fixedsize(fptu_tag_t tag) cxx11_noexcept;
FPTU_API bool fptu_tag_is_deleted(fptu_tag_t tag) cxx11_noexcept;

//------------------------------------------------------------------------------

/* Возвращает минимальный размер буфера, который необходим для размещения
 * кортежа с указанным кол-вом полей и данных. */
FPTU_API std::size_t fptu_space(size_t items,
                                std::size_t data_bytes) cxx11_noexcept;

/* Инициализирует в буфере кортеж, резервируя (отступая) место достаточное
 * для добавления до items_limit полей. Оставшееся место будет
 * использоваться для размещения данных.
 *
 * Возвращает указатель на созданный в буфере объект, либо nullptr, если
 * заданы неверные параметры или размер буфера недостаточен. Если для созданного
 * объекта вызвать fptu_destroy(), то будет предстринята попытка освободить
 * буфер посредством free(). */
FPTU_API fptu_rw *fptu_init(void *buffer_space, std::size_t buffer_bytes,
                            std::size_t items_limit) cxx11_noexcept;

/* Выделяет и инициализирует кортеж достаточный для размещения заданного кол-ва
 * полей и данных.
 *
 * Возвращает адрес объекта, который необходимо разрушить поспедством
 * fptu_destroy() по окончании использования. Либо nullptr при неверных
 * параметрах или нехватке памяти. */
FPTU_API fptu_rw *fptu_alloc(size_t items_limit,
                             std::size_t data_bytes) cxx11_noexcept;
FPTU_API void fptu_destroy(fptu_rw *pt) cxx11_noexcept;

/* Очищает ранее инициализированный кортеж.
 * В случае успеха возвращает ноль, иначе код ошибки. */
FPTU_API fptu_error fptu_clear(fptu_rw *pt) cxx11_noexcept;

/* Возвращает кол-во свободных слотов для добавления дескрипторов
 * полей в заголовок кортежа. */
FPTU_API std::size_t fptu_space4items(const fptu_rw *pt) cxx11_noexcept;

/* Возвращает остаток места доступного для размещения данных. */
FPTU_API std::size_t fptu_space4data(const fptu_rw *pt) cxx11_noexcept;

/* Возвращает объем мусора в байтах, который станет доступным для
 * добавления полей и данных после fptu_shrink(). */
FPTU_API std::size_t fptu_junkspace(const fptu_rw *pt) cxx11_noexcept;

/* Проверяет сериализованную форму кортежа на корректность.
 *
 * Возвращает nullptr если ошибок не обнаружено, либо указатель на константную
 * строку с краткой информацией о проблеме (нарушенное условие). */
FPTU_API const char *
fptu_check_ro_ex(fptu_ro ro, bool holes_are_not_allowed) cxx11_noexcept;
static inline const char *fptu_check_ro(fptu_ro ro) cxx11_noexcept {
  return fptu_check_ro_ex(ro, false);
}

/* Проверяет модифицируемую форму кортежа на корректность.
 *
 * Возвращает nullptr если ошибок не обнаружено, либо указатель на константную
 * строку с краткой информацией о проблеме (нарушенное условие). */
FPTU_API const char *fptu_check_rw(const fptu_rw *pt) cxx11_noexcept;

/* Возвращает сериализованную форму кортежа, которая находится внутри
 * модифицируемой. Дефрагментация не выполняется, поэтому сериализованная
 * форма может содержать лишний мусор, см fptu_junkspace().
 *
 * Возвращаемый результат валиден до изменения или разрушения исходной
 * модифицируемой формы кортежа. */
FPTU_API fptu_ro fptu_take_noshrink(const fptu_rw *pt) cxx11_noexcept;

/* Проверяет содержимое сериализованной формы на корректность. Одновременно
 * возвращает размер буфера, который потребуется для модифицируемой формы,
 * с учетом добавляемых more_items полей и more_payload данных.
 *
 * При неверных параметрах или некорректных данных возвращает 0 и записывает
 * в error информацию об ошибке (указатель на константную строку
 * с краткой информацией о проблеме). */
struct fptu_cbfs_result {
  const char *err_msg;
  fptu_error err;
  unsigned short holes_count, holes_volume;
};

FPTU_API std::size_t
fptu_check_and_get_buffer_size_ex(fptu_ro ro, unsigned more_items,
                                  unsigned more_payload,
                                  struct fptu_cbfs_result *cbfs) cxx11_noexcept;

FPTU_API std::size_t
fptu_check_and_get_buffer_size(fptu_ro ro, unsigned more_items,
                               unsigned more_payload,
                               const char **error) cxx11_noexcept;

FPTU_API std::size_t fptu_get_buffer_size(fptu_ro ro, unsigned more_items,
                                          unsigned more_payload) cxx11_noexcept;

/* Строит в указанном буфере модифицируемую форму кортежа из сериализованной.
 * Проверка корректности данных в сериализованной форме не производится.
 * Сериализованная форма не модифицируется и не требуется после возврата из
 * функции.
 * Аргумент more_items резервирует кол-во полей, которые можно будет добавить
 * в создаваемую модифицируемую форму.
 *
 * Возвращает указатель на созданный в буфере объект, либо nullptr, если
 * заданы неверные параметры или размер буфера недостаточен. */
FPTU_API fptu_rw *fptu_fetch(fptu_ro ro, void *buffer_space,
                             std::size_t buffer_bytes,
                             unsigned more_items) cxx11_noexcept;
FPTU_API fptu_rw *fptu_fetch_ex(fptu_ro ro, void *buffer_space,
                                std::size_t buffer_bytes, unsigned more_items,
                                struct fptu_cbfs_result *cbfs) cxx11_noexcept;

/* Производит дефрагментацию модифицируемой формы кортежа.
 * Возвращает true если была произведена дефрагментация, что можно
 * использовать как признак инвалидации итераторов. */
FPTU_API bool fptu_shrink(fptu_rw *pt) cxx11_noexcept;

/* Производит дефрагментацию модифицируемой формы кортежа при наличии
 * пустот/мусора после удаления полей.
 * Возвращает true если была произведена дефрагментация, что можно
 * использовать как признак инвалидации итераторов. */
static __inline bool fptu_cond_shrink(fptu_rw *pt) {
  return fptu_junkspace(pt) > 0 && fptu_shrink(pt);
}

/* Возвращает сериализованную форму кортежа, которая находится внутри
 * модифицируемой. При необходимости автоматически производится
 * дефрагментация.
 * Возвращаемый результат валиден до изменения или разрушения исходной
 * модифицируемой формы кортежа. */
static __inline fptu_ro fptu_take(fptu_rw *pt) {
  fptu_cond_shrink(pt);
  return fptu_take_noshrink(pt);
}

/* Если в аргументе type_or_filter взведен бит fptu_flag_filter,
 * то type_or_filter интерпретируется как битовая маска типов.
 * Соответственно, будут удалены все поля с заданным column и попадающие
 * в маску типов. Например, если type_or_filter равен fptu_any_fp,
 * то удаляются все fptu_fp32 и fptu_fp64.
 *
 * Из C++ в namespace fptu доступно несколько перегруженных вариантов.
 *
 * Возвращается кол-во удаленных полей (больше либо равно нулю),
 * либо отрицательный код ошибки. */
FPTU_API int fptu_erase(fptu_rw *pt, unsigned column,
                        fptu_type_or_filter type_or_filter) cxx11_noexcept;
FPTU_API void fptu_erase_field(fptu_rw *pt, fptu_field *pf) cxx11_noexcept;

//------------------------------------------------------------------------------

FPTU_API bool fptu_is_empty_ro(fptu_ro ro) cxx11_noexcept;
FPTU_API bool fptu_is_empty_rw(const fptu_rw *pt) cxx11_noexcept;

/* Возвращает первое поле попадающее под критерий выбора, либо nullptr.
 * Семантика type_or_filter указана в описании fptu_erase().
 *
 * Из C++ в namespace fptu доступно несколько перегруженных вариантов. */
FPTU_API const fptu_field *
fptu_lookup_ro(fptu_ro ro, unsigned column,
               fptu_type_or_filter type_or_filter) cxx11_noexcept;
FPTU_API fptu_field *
fptu_lookup_rw(fptu_rw *pt, unsigned column,
               fptu_type_or_filter type_or_filter) cxx11_noexcept;

/* Возвращает "итераторы" по кортежу, в виде указателей.
 * Гарантируется что begin меньше, либо равно end.
 * В возвращаемом диапазоне могут буть удаленные поля,
 * для которых fptu_field_column() возвращает отрицательное значение. */
FPTU_API const fptu_field *fptu_begin_ro(fptu_ro ro) cxx11_noexcept;
FPTU_API const fptu_field *fptu_end_ro(fptu_ro ro) cxx11_noexcept;
FPTU_API const fptu_field *fptu_begin_rw(const fptu_rw *pt) cxx11_noexcept;
FPTU_API const fptu_field *fptu_end_rw(const fptu_rw *pt) cxx11_noexcept;

/* Итерация полей кортежа с заданным условие отбора, при этом
 * удаленные поля пропускаются.
 * Семантика type_or_filter указана в описании fptu_erase().
 *
 * Из C++ в namespace fptu доступно несколько перегруженных вариантов. */
FPTU_API const fptu_field *
fptu_first(const fptu_field *begin, const fptu_field *end, unsigned column,
           fptu_type_or_filter type_or_filter) cxx11_noexcept;
FPTU_API const fptu_field *
fptu_next(const fptu_field *from, const fptu_field *end, unsigned column,
          fptu_type_or_filter type_or_filter) cxx11_noexcept;

/* Итерация полей кортежа с заданным внешним фильтром, при этом
 * удаленные поля пропускаются. */
typedef bool fptu_field_filter(const fptu_field *, void *context,
                               void *param) cxx17_noexcept;
FPTU_API const fptu_field *fptu_first_ex(const fptu_field *begin,
                                         const fptu_field *end,
                                         fptu_field_filter filter,
                                         void *context,
                                         void *param) cxx11_noexcept;
FPTU_API const fptu_field *fptu_next_ex(const fptu_field *from,
                                        const fptu_field *end,
                                        fptu_field_filter filter, void *context,
                                        void *param) cxx11_noexcept;

/* Подсчет количества полей по заданному номеру колонки и типу,
 * либо маски типов.
 * Семантика type_or_filter указана в описании fptu_erase().
 *
 * Из C++ в namespace fptu доступно несколько перегруженных вариантов. */
FPTU_API std::size_t
fptu_field_count_ro(fptu_ro ro, unsigned column,
                    fptu_type_or_filter type_or_filter) cxx11_noexcept;

FPTU_API std::size_t
fptu_field_count_rw(const fptu_rw *rw, unsigned column,
                    fptu_type_or_filter type_or_filter) cxx11_noexcept;

/* Подсчет количества полей задаваемой функцией-фильтром. */
FPTU_API std::size_t fptu_field_count_rw_ex(const fptu_rw *rw,
                                            fptu_field_filter filter,
                                            void *context,
                                            void *param) cxx11_noexcept;
FPTU_API std::size_t fptu_field_count_ro_ex(fptu_ro ro,
                                            fptu_field_filter filter,
                                            void *context,
                                            void *param) cxx11_noexcept;

FPTU_API fptu_type fptu_field_type(const fptu_field *pf) cxx11_noexcept;
FPTU_API int fptu_field_column(const fptu_field *pf) cxx11_noexcept;
FPTU_API iovec fptu_field_as_iovec(const fptu_field *pf) cxx11_noexcept;
FPTU_API bool fptu_field_is_dead(const fptu_field *pf) cxx11_noexcept;
static __inline const void *fptu_field_payload(const fptu_field *pf) {
  return fptu_field_as_iovec(pf).iov_base;
}

FPTU_API uint_fast16_t fptu_field_uint16(const fptu_field *pf) cxx11_noexcept;
FPTU_API bool fptu_field_bool(const fptu_field *pf) cxx11_noexcept;
FPTU_API int_fast32_t fptu_field_int32(const fptu_field *pf) cxx11_noexcept;
FPTU_API uint_fast32_t fptu_field_uint32(const fptu_field *pf) cxx11_noexcept;
FPTU_API int_fast64_t fptu_field_int64(const fptu_field *pf) cxx11_noexcept;
FPTU_API uint_fast64_t fptu_field_uint64(const fptu_field *pf) cxx11_noexcept;
FPTU_API double fptu_field_fp64(const fptu_field *pf) cxx11_noexcept;
FPTU_API float fptu_field_fp32(const fptu_field *pf) cxx11_noexcept;
FPTU_API union fptu_datetime_C
fptu_field_datetime(const fptu_field *pf) cxx11_noexcept;
FPTU_API const uint8_t *fptu_field_96(const fptu_field *pf) cxx11_noexcept;
FPTU_API const uint8_t *fptu_field_128(const fptu_field *pf) cxx11_noexcept;
FPTU_API const uint8_t *fptu_field_160(const fptu_field *pf) cxx11_noexcept;
FPTU_API const uint8_t *fptu_field_256(const fptu_field *pf) cxx11_noexcept;
FPTU_API const char *fptu_field_cstr(const fptu_field *pf) cxx11_noexcept;
FPTU_API struct iovec fptu_field_opaque(const fptu_field *pf) cxx11_noexcept;
FPTU_API fptu_ro fptu_field_nested(const fptu_field *pf) cxx11_noexcept;

FPTU_API uint_fast16_t fptu_get_uint16(fptu_ro ro, unsigned column,
                                       int *error) cxx11_noexcept;
FPTU_API bool fptu_get_bool(fptu_ro ro, unsigned column,
                            int *error) cxx11_noexcept;
FPTU_API int_fast32_t fptu_get_int32(fptu_ro ro, unsigned column,
                                     int *error) cxx11_noexcept;
FPTU_API uint_fast32_t fptu_get_uint32(fptu_ro ro, unsigned column,
                                       int *error) cxx11_noexcept;
FPTU_API int_fast64_t fptu_get_int64(fptu_ro ro, unsigned column,
                                     int *error) cxx11_noexcept;
FPTU_API uint_fast64_t fptu_get_uint64(fptu_ro ro, unsigned column,
                                       int *error) cxx11_noexcept;
FPTU_API double fptu_get_fp64(fptu_ro ro, unsigned column,
                              int *error) cxx11_noexcept;
FPTU_API float fptu_get_fp32(fptu_ro ro, unsigned column,
                             int *error) cxx11_noexcept;
FPTU_API union fptu_datetime_C fptu_get_datetime(fptu_ro ro, unsigned column,
                                                 int *error) cxx11_noexcept;

FPTU_API const uint8_t *fptu_get_96(fptu_ro ro, unsigned column,
                                    int *error) cxx11_noexcept;
FPTU_API const uint8_t *fptu_get_128(fptu_ro ro, unsigned column,
                                     int *error) cxx11_noexcept;
FPTU_API const uint8_t *fptu_get_160(fptu_ro ro, unsigned column,
                                     int *error) cxx11_noexcept;
FPTU_API const uint8_t *fptu_get_256(fptu_ro ro, unsigned column,
                                     int *error) cxx11_noexcept;
FPTU_API const char *fptu_get_cstr(fptu_ro ro, unsigned column,
                                   int *error) cxx11_noexcept;
FPTU_API struct iovec fptu_get_opaque(fptu_ro ro, unsigned column,
                                      int *error) cxx11_noexcept;
FPTU_API fptu_ro fptu_get_nested(fptu_ro ro, unsigned column,
                                 int *error) cxx11_noexcept;

/* TODO */
FPTU_API int_fast64_t fptu_get_sint(fptu_ro ro, unsigned column,
                                    int *error) cxx11_noexcept;
FPTU_API uint_fast64_t fptu_get_uint(fptu_ro ro, unsigned column,
                                     int *error) cxx11_noexcept;
FPTU_API double fptu_get_fp(fptu_ro ro, unsigned column,
                            int *error) cxx11_noexcept;

//------------------------------------------------------------------------------

/* Вставка или обновление существующего поля.
 *
 * В случае коллекций, когда в кортеже более одного поля с соответствующим
 * типом и номером), будет обновлен первый найденный экземпляр. Но так как
 * в общем случае физический порядок полей не определен, следует считать что
 * функция обновит произвольный экземпляр поля. Поэтому для манипулирования
 * коллекциями следует использовать fptu_erase() и/или fput_field_set_xyz(). */
FPTU_API fptu_error fptu_upsert_null(fptu_rw *pt,
                                     unsigned column) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_uint16(fptu_rw *pt, unsigned column,
                                       uint16_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_bool(fptu_rw *pt, unsigned column,
                                     bool value) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_int32(fptu_rw *pt, unsigned column,
                                      int_fast32_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_uint32(fptu_rw *pt, unsigned column,
                                       uint_fast32_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_int64(fptu_rw *pt, unsigned column,
                                      int_fast64_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_uint64(fptu_rw *pt, unsigned column,
                                       uint_fast64_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_fp64(fptu_rw *pt, unsigned column,
                                     double value) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_fp32(fptu_rw *pt, unsigned column,
                                     float value) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_datetime(fptu_rw *pt, unsigned column,
                                         const fptu_datetime_t) cxx11_noexcept;

FPTU_API fptu_error fptu_upsert_96(fptu_rw *pt, unsigned column,
                                   const void *data) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_128(fptu_rw *pt, unsigned column,
                                    const void *data) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_160(fptu_rw *pt, unsigned column,
                                    const void *data) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_256(fptu_rw *pt, unsigned column,
                                    const void *data) cxx11_noexcept;

FPTU_API fptu_error fptu_upsert_string(fptu_rw *pt, unsigned column,
                                       const char *text,
                                       std::size_t length) cxx11_noexcept;
static __inline fptu_error fptu_upsert_cstr(fptu_rw *pt, unsigned column,
                                            const char *value) {
  return fptu_upsert_string(pt, column, value, value ? strlen(value) : 0);
}

FPTU_API fptu_error fptu_upsert_opaque(fptu_rw *pt, unsigned column,
                                       const void *data,
                                       std::size_t bytes) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_opaque_iov(
    fptu_rw *pt, unsigned column, const struct iovec value) cxx11_noexcept;
FPTU_API fptu_error fptu_upsert_nested(fptu_rw *pt, unsigned column,
                                       fptu_ro ro) cxx11_noexcept;

//------------------------------------------------------------------------------

// Добавление ещё одного поля, для поддержки коллекций.
FPTU_API fptu_error fptu_insert_uint16(fptu_rw *pt, unsigned column,
                                       uint16_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_bool(fptu_rw *pt, unsigned column,
                                     bool value) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_int32(fptu_rw *pt, unsigned column,
                                      int_fast32_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_uint32(fptu_rw *pt, unsigned column,
                                       uint_fast32_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_int64(fptu_rw *pt, unsigned column,
                                      int_fast64_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_uint64(fptu_rw *pt, unsigned column,
                                       uint_fast64_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_fp64(fptu_rw *pt, unsigned column,
                                     double value) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_fp32(fptu_rw *pt, unsigned column,
                                     float value) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_datetime(fptu_rw *pt, unsigned column,
                                         const fptu_datetime_t) cxx11_noexcept;

FPTU_API fptu_error fptu_insert_96(fptu_rw *pt, unsigned column,
                                   const void *data) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_128(fptu_rw *pt, unsigned column,
                                    const void *data) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_160(fptu_rw *pt, unsigned column,
                                    const void *data) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_256(fptu_rw *pt, unsigned column,
                                    const void *data) cxx11_noexcept;

FPTU_API fptu_error fptu_insert_string(fptu_rw *pt, unsigned column,
                                       const char *text,
                                       std::size_t length) cxx11_noexcept;
static __inline fptu_error fptu_insert_cstr(fptu_rw *pt, unsigned column,
                                            const char *value) {
  return fptu_insert_string(pt, column, value, value ? strlen(value) : 0);
}

FPTU_API fptu_error fptu_insert_opaque(fptu_rw *pt, unsigned column,
                                       const void *value,
                                       std::size_t bytes) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_opaque_iov(
    fptu_rw *pt, unsigned column, const struct iovec value) cxx11_noexcept;
FPTU_API fptu_error fptu_insert_nested(fptu_rw *pt, unsigned column,
                                       fptu_ro ro) cxx11_noexcept;

//------------------------------------------------------------------------------

// Обновление существующего поля или первого найденного элемента коллекции.
FPTU_API fptu_error fptu_update_uint16(fptu_rw *pt, unsigned column,
                                       uint16_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_update_bool(fptu_rw *pt, unsigned column,
                                     bool value) cxx11_noexcept;
FPTU_API fptu_error fptu_update_int32(fptu_rw *pt, unsigned column,
                                      int_fast32_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_update_uint32(fptu_rw *pt, unsigned column,
                                       uint_fast32_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_update_int64(fptu_rw *pt, unsigned column,
                                      int_fast64_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_update_uint64(fptu_rw *pt, unsigned column,
                                       uint_fast64_t value) cxx11_noexcept;
FPTU_API fptu_error fptu_update_fp64(fptu_rw *pt, unsigned column,
                                     double value) cxx11_noexcept;
FPTU_API fptu_error fptu_update_fp32(fptu_rw *pt, unsigned column,
                                     float value) cxx11_noexcept;
FPTU_API fptu_error fptu_update_datetime(fptu_rw *pt, unsigned column,
                                         const fptu_datetime_t) cxx11_noexcept;

FPTU_API fptu_error fptu_update_96(fptu_rw *pt, unsigned column,
                                   const void *data) cxx11_noexcept;
FPTU_API fptu_error fptu_update_128(fptu_rw *pt, unsigned column,
                                    const void *data) cxx11_noexcept;
FPTU_API fptu_error fptu_update_160(fptu_rw *pt, unsigned column,
                                    const void *data) cxx11_noexcept;
FPTU_API fptu_error fptu_update_256(fptu_rw *pt, unsigned column,
                                    const void *data) cxx11_noexcept;

FPTU_API fptu_error fptu_update_string(fptu_rw *pt, unsigned column,
                                       const char *text,
                                       std::size_t length) cxx11_noexcept;
static __inline fptu_error fptu_update_cstr(fptu_rw *pt, unsigned column,
                                            const char *value) {
  return fptu_update_string(pt, column, value, value ? strlen(value) : 0);
}

FPTU_API fptu_error fptu_update_opaque(fptu_rw *pt, unsigned column,
                                       const void *value,
                                       std::size_t bytes) cxx11_noexcept;
FPTU_API fptu_error fptu_update_opaque_iov(
    fptu_rw *pt, unsigned column, const struct iovec value) cxx11_noexcept;
FPTU_API fptu_error fptu_update_nested(fptu_rw *pt, unsigned column,
                                       fptu_ro ro) cxx11_noexcept;

//------------------------------------------------------------------------------
/* Определения и примитивы для сравнения. */

typedef enum fptu_lge {
  fptu_ic = 1,                           // incomparable
  fptu_eq = 2,                           // left == right
  fptu_lt = 4,                           // left < right
  fptu_gt = 8,                           // left > right
  fptu_ne = fptu_lt | fptu_gt | fptu_ic, // left != right
  fptu_le = fptu_lt | fptu_eq,           // left <= right
  fptu_ge = fptu_gt | fptu_eq            // left >= right
} fptu_lge;

FPTU_API fptu_lge fptu_cmp_96(fptu_ro ro, unsigned column,
                              const uint8_t *value) cxx11_noexcept;
FPTU_API fptu_lge fptu_cmp_128(fptu_ro ro, unsigned column,
                               const uint8_t *value) cxx11_noexcept;
FPTU_API fptu_lge fptu_cmp_160(fptu_ro ro, unsigned column,
                               const uint8_t *value) cxx11_noexcept;
FPTU_API fptu_lge fptu_cmp_256(fptu_ro ro, unsigned column,
                               const uint8_t *value) cxx11_noexcept;
FPTU_API fptu_lge fptu_cmp_opaque(fptu_ro ro, unsigned column,
                                  const void *value,
                                  std::size_t bytes) cxx11_noexcept;
FPTU_API fptu_lge fptu_cmp_opaque_iov(fptu_ro ro, unsigned column,
                                      const struct iovec value) cxx11_noexcept;

FPTU_API fptu_lge fptu_cmp_binary(const void *left_data, std::size_t left_len,
                                  const void *right_data,
                                  std::size_t right_len) cxx11_noexcept;

FPTU_API fptu_lge fptu_cmp_fields(const fptu_field *left,
                                  const fptu_field *right) cxx11_noexcept;
FPTU_API fptu_lge fptu_cmp_tuples(fptu_ro left, fptu_ro right) cxx11_noexcept;

//------------------------------------------------------------------------------

/* Сериализует JSON-представление кортежа в "толкающей" (aka push) модели,
 * выталкивая генерируемый текст в предоставленную функцию.
 * Для трансляции идентификаторов/тегов полей и enum-значений (fptu_uint16)
 * в символические имена используются передаваемые в параметрах функции.
 *
 * Параметры output и output_ctx используются для вывода результирующих
 * текстовых данных, и поэтому обязательны. При вызове output в качестве
 * первого параметра будет передан output_ctx.
 *
 * Параметры indent и depth задают отступ и начальный уровень вложенности.
 * Оба этих параметра опциональные, т.е. могут быть нулевыми.
 *
 * Параметры schema_ctx, tag2name и value2enum используются для трансляции
 * идентификаторов/тегов и значений в символические имена, т.е. выполняют
 * роль примитивного справочника схемы. Все три параметра опциональны и могу
 * быть нулевыми, но при этом вместо символических имен для сериализации будут
 * использованы числовые идентификаторы.
 *
 * В случае успеха возвращает ноль, иначе код ошибки. */
FPTU_API fptu_error
fptu_tuple2json(fptu_ro tuple, fptu_emit_func output, void *output_ctx,
                const char *indent, unsigned depth, const void *schema_ctx,
                fptu_tag2name_func tag2name, fptu_value2enum_func value2enum,
                const fptu_json_options options) cxx11_noexcept;

/* Сериализует JSON-представление кортежа в FILE.
 *
 * Назначение параметров indent, depth, schema_ctx, tag2name и value2enum
 * см в описании функции fptu_tuple2json().
 *
 * В случае успеха возвращает ноль, иначе код ошибки, в том числе
 * значение errno в случае ошибки записи в FILE */
FPTU_API fptu_error fptu_tuple2json_FILE(
    fptu_ro tuple, FILE *file, const char *indent, unsigned depth,
    const void *schema_ctx, fptu_tag2name_func tag2name,
    fptu_value2enum_func value2enum,
    const fptu_json_options options) cxx11_noexcept;

//------------------------------------------------------------------------------
/* Некоторые внутренние служебные функции.
 * Доступны для специальных случаев, в том числе для тестов. */

FPTU_API bool fptu_is_ordered(const fptu_field *begin,
                              const fptu_field *end) cxx11_noexcept;
FPTU_API uint16_t *fptu_tags(uint16_t *const first, const fptu_field *begin,
                             const fptu_field *end) cxx11_noexcept;
FPTU_API bool fptu_is_under_valgrind(void) cxx11_noexcept;
FPTU_API const char *fptu_type_name(const fptu_type) cxx11_noexcept;

#ifdef __cplusplus
} /* extern "C" */

//------------------------------------------------------------------------------
/* Сервисные функции и классы для C++ */

namespace fptu {

using tuple_ptr = tuple_rw_managed;

inline int erase(fptu_rw *pt, unsigned column, fptu_type type) {
  return fptu_erase(pt, column, fptu_type_or_filter(type));
}

inline int erase(fptu_rw *pt, unsigned column, fptu_filter filter) {
  return fptu_erase(pt, column, fptu_type_or_filter(filter));
}

inline void erase(fptu_rw *pt, fptu_field *pf) { fptu_erase_field(pt, pf); }

inline bool is_empty(const fptu_ro &ro) { return fptu_is_empty_ro(ro); }

inline bool is_empty(const fptu_rw *pt) { return fptu_is_empty_rw(pt); }

inline const char *check(const fptu_ro &ro) { return fptu_check_ro(ro); }

inline const char *check(const fptu_rw *pt) { return fptu_check_rw(pt); }

inline const fptu_field *lookup(const fptu_ro &ro, unsigned column,
                                fptu_type type) {
  return fptu_lookup_ro(ro, column, fptu_type_or_filter(type));
}
inline const fptu_field *lookup(const fptu_ro &ro, unsigned column,
                                fptu_filter filter) {
  return fptu_lookup_ro(ro, column, fptu_type_or_filter(filter));
}

inline fptu_field *lookup(fptu_rw *rw, unsigned column, fptu_type type) {
  return fptu_lookup_rw(rw, column, fptu_type_or_filter(type));
}
inline fptu_field *lookup(fptu_rw *rw, unsigned column, fptu_filter filter) {
  return fptu_lookup_rw(rw, column, fptu_type_or_filter(filter));
}

inline const fptu_field *begin(const fptu_ro &ro) { return fptu_begin_ro(ro); }

inline const fptu_field *begin(const fptu_ro *ro) { return fptu_begin_ro(*ro); }

inline const fptu_field *begin(const fptu_rw &rw) { return fptu_begin_rw(&rw); }

inline const fptu_field *begin(const fptu_rw *rw) { return fptu_begin_rw(rw); }

inline const fptu_field *end(const fptu_ro &ro) { return fptu_end_ro(ro); }

inline const fptu_field *end(const fptu_ro *ro) { return fptu_end_ro(*ro); }

inline const fptu_field *end(const fptu_rw &rw) { return fptu_end_rw(&rw); }

inline const fptu_field *end(const fptu_rw *rw) { return fptu_end_rw(rw); }

inline const fptu_field *first(const fptu_field *begin, const fptu_field *end,
                               unsigned column, fptu_type type) {
  return fptu_first(begin, end, column, fptu_type_or_filter(type));
}
inline const fptu_field *first(const fptu_field *begin, const fptu_field *end,
                               unsigned column, fptu_filter filter) {
  return fptu_first(begin, end, column, fptu_type_or_filter(filter));
}

inline const fptu_field *next(const fptu_field *from, const fptu_field *end,
                              unsigned column, fptu_type type) {
  return fptu_next(from, end, column, fptu_type_or_filter(type));
}
inline const fptu_field *next(const fptu_field *from, const fptu_field *end,
                              unsigned column, fptu_filter filter) {
  return fptu_next(from, end, column, fptu_type_or_filter(filter));
}

inline const fptu_field *first(const fptu_field *begin, const fptu_field *end,
                               fptu_field_filter filter, void *context,
                               void *param) {
  return fptu_first_ex(begin, end, filter, context, param);
}

inline const fptu_field *next(const fptu_field *begin, const fptu_field *end,
                              fptu_field_filter filter, void *context,
                              void *param) {
  return fptu_next_ex(begin, end, filter, context, param);
}

inline std::size_t field_count(const fptu_ro &ro, unsigned column,
                               fptu_type type) {
  return fptu_field_count_ro(ro, column, fptu_type_or_filter(type));
}
inline std::size_t field_count(const fptu_ro &ro, unsigned column,
                               fptu_filter filter) {
  return fptu_field_count_ro(ro, column, fptu_type_or_filter(filter));
}

inline std::size_t field_count(const fptu_rw *rw, unsigned column,
                               fptu_type type) {
  return fptu_field_count_rw(rw, column, fptu_type_or_filter(type));
}
inline std::size_t field_count(const fptu_rw *rw, unsigned column,
                               fptu_filter filter) {
  return fptu_field_count_rw(rw, column, fptu_type_or_filter(filter));
}

inline std::size_t field_count(const fptu_rw *rw, fptu_field_filter filter,
                               void *context, void *param) {
  return fptu_field_count_rw_ex(rw, filter, context, param);
}

inline std::size_t field_count(const fptu_ro &ro, fptu_field_filter filter,
                               void *context, void *param) {
  return fptu_field_count_ro_ex(ro, filter, context, param);
}

inline std::size_t check_and_get_buffer_size(const fptu_ro &ro,
                                             unsigned more_items,
                                             unsigned more_payload,
                                             const char **error) {
  return fptu_check_and_get_buffer_size(ro, more_items, more_payload, error);
}

inline std::size_t get_buffer_size(const fptu_ro &ro, unsigned more_items,
                                   unsigned more_payload) {
  return fptu_get_buffer_size(ro, more_items, more_payload);
}

#if 0
static inline int64_t cast_wide(int8_t value) { return value; }
static inline int64_t cast_wide(int16_t value) { return value; }
static inline int64_t cast_wide(int32_t value) { return value; }
static inline int64_t cast_wide(int64_t value) { return value; }
static inline uint64_t cast_wide(uint8_t value) { return value; }
static inline uint64_t cast_wide(uint16_t value) { return value; }
static inline uint64_t cast_wide(uint32_t value) { return value; }
static inline uint64_t cast_wide(uint64_t value) { return value; }
static inline double_t cast_wide(float value) { return value; }
static inline double_t cast_wide(double value) { return value; }
#if defined(LDBL_MANT_DIG) && defined(LDBL_MAX_EXP) &&                         \
    (LDBL_MANT_DIG != DBL_MANT_DIG || LDBL_MAX_EXP != DBL_MAX_EXP)
static inline double_t cast_wide(long double value) { return value; }
#endif /* long double */

template <typename VALUE_TYPE, typename RANGE_BEGIN_TYPE,
          typename RANGE_END_TYPE>
inline bool is_within(VALUE_TYPE value, RANGE_BEGIN_TYPE begin,
                      RANGE_END_TYPE end) {
  return is_within(cast_wide(value), cast_wide(begin), cast_wide(end));
}

template <>
inline bool is_within<int64_t, int64_t, int64_t>(int64_t value, int64_t begin,
                                                 int64_t end) {
  assert(begin < end);
  return value >= begin && value <= end;
}

template <>
inline bool is_within<uint64_t, uint64_t, uint64_t>(uint64_t value,
                                                    uint64_t begin,
                                                    uint64_t end) {
  assert(begin < end);
  return value >= begin && value <= end;
}

template <>
inline bool is_within<double_t, double_t, double_t>(double_t value,
                                                    double_t begin,
                                                    double_t end) {
  assert(begin < end);
  return value >= begin && value <= end;
}

template <>
inline bool is_within<uint64_t, int64_t, int64_t>(uint64_t value, int64_t begin,
                                                  int64_t end) {
  assert(begin < end);
  if (end < 0 || value > uint64_t(end))
    return false;
  if (begin > 0 && value < uint64_t(begin))
    return false;
  return true;
}

template <>
inline bool is_within<uint64_t, double_t, double_t>(uint64_t value,
                                                    double_t begin,
                                                    double_t end) {
  assert(begin < end);
  if (end < 0 || (end < double_t(UINT64_MAX) && value > uint64_t(end)))
    return false;
  if (begin > 0 && (begin > double_t(UINT64_MAX) || value < uint64_t(begin)))
    return false;
  return true;
}

template <>
inline bool is_within<int64_t, double_t, double_t>(int64_t value,
                                                   double_t begin,
                                                   double_t end) {
  assert(begin < end);
  if (end < double_t(INT64_MAX) && value > int64_t(end))
    return false;
  if (begin > double_t(INT64_MAX) || value < int64_t(begin))
    return false;
  return true;
}

template <>
inline bool is_within<int64_t, uint64_t, uint64_t>(int64_t value,
                                                   uint64_t begin,
                                                   uint64_t end) {
  assert(begin < end);
  if (value < 0)
    return false;
  return is_within(uint64_t(value), begin, end);
}

template <>
inline bool is_within<double_t, int64_t, int64_t>(double_t value, int64_t begin,
                                                  int64_t end) {
  assert(begin < end);
  return value >= begin && value <= end;
}

template <>
inline bool is_within<double_t, uint64_t, uint64_t>(double_t value,
                                                    uint64_t begin,
                                                    uint64_t end) {
  assert(begin < end);
  return value >= begin && value <= end;
}

template <fptu_type field_type, typename RESULT_TYPE>
static RESULT_TYPE get_number(const fptu_field *field) {
  assert(field != nullptr);
  static_assert(fptu_any_number & (INT32_C(1) << field_type),
                "field_type must be numerical");
  assert(field->tag);
  switch (field_type) {
  default:
    assert(false);
    return 0;
  case fptu_uint16:
    assert(is_within(field->get_payload_uint16(),
                     std::numeric_limits<RESULT_TYPE>::lowest(),
                     std::numeric_limits<RESULT_TYPE>::max()));
    return RESULT_TYPE(field->get_payload_uint16());
  case fptu_uint32:
    assert(is_within(field->payload()->u32,
                     std::numeric_limits<RESULT_TYPE>::lowest(),
                     std::numeric_limits<RESULT_TYPE>::max()));
    return RESULT_TYPE(field->payload()->u32);
  case fptu_uint64:
    assert(is_within(field->payload()->u64,
                     std::numeric_limits<RESULT_TYPE>::lowest(),
                     std::numeric_limits<RESULT_TYPE>::max()));
    return RESULT_TYPE(field->payload()->u64);
  case fptu_int32:
    assert(is_within(field->payload()->i32,
                     std::numeric_limits<RESULT_TYPE>::lowest(),
                     std::numeric_limits<RESULT_TYPE>::max()));
    return RESULT_TYPE(field->payload()->i32);
  case fptu_int64:
    assert(is_within(field->payload()->i64,
                     std::numeric_limits<RESULT_TYPE>::lowest(),
                     std::numeric_limits<RESULT_TYPE>::max()));
    return RESULT_TYPE(field->payload()->i64);
  case fptu_fp32:
    assert(is_within(field->payload()->fp32,
                     std::numeric_limits<RESULT_TYPE>::lowest(),
                     std::numeric_limits<RESULT_TYPE>::max()));
    return RESULT_TYPE(field->payload()->fp32);
  case fptu_fp64:
    assert(is_within(field->payload()->fp32,
                     std::numeric_limits<RESULT_TYPE>::lowest(),
                     std::numeric_limits<RESULT_TYPE>::max()));
    return RESULT_TYPE(field->payload()->fp64);
  }
}

template <fptu_type field_type, typename VALUE_TYPE>
static void set_number(fptu_field *field, const VALUE_TYPE &value) {
  assert(field != nullptr);
  static_assert(fptu_any_number & (INT32_C(1) << field_type),
                "field_type must be numerical");
  switch (field_type) {
  default:
    assert(false);
    break;
  case fptu_uint16:
    assert(is_within(value, 0, INT16_MAX));
    field->offset = uint16_t(value);
    break;
  case fptu_uint32:
    assert(is_within(value, 0u, UINT32_MAX));
    field->payload()->u32 = uint32_t(value);
    break;
  case fptu_uint64:
    assert(is_within(value, 0u, UINT64_MAX));
    field->payload()->u64 = uint64_t(value);
    break;
  case fptu_int32:
    assert(is_within(value, INT32_MIN, INT32_MAX));
    field->payload()->i32 = int32_t(value);
    break;
  case fptu_int64:
    assert(is_within(value, INT64_MIN, INT64_MAX));
    field->payload()->i64 = int64_t(value);
    break;
  case fptu_fp32:
    assert(value >= FLT_MIN && value <= FLT_MAX);
    field->payload()->fp32 = float(value);
    break;
  case fptu_fp64:
    assert(value >= DBL_MIN && value <= DBL_MAX);
    field->payload()->fp64 = double(value);
    break;
  }
}

template <fptu_type field_type, typename VALUE_TYPE>
static int upsert_number(fptu_rw *pt, unsigned colnum,
                         const VALUE_TYPE &value) {
  static_assert(fptu_any_number & (INT32_C(1) << field_type),
                "field_type must be numerical");
  switch (field_type) {
  default:
    assert(false);
    return 0;
  case fptu_uint16:
    assert(is_within(value, 0, INT16_MAX));
    return fptu_upsert_uint16(pt, colnum, uint_fast16_t(value));
  case fptu_uint32:
    assert(is_within(value, 0u, UINT32_MAX));
    return fptu_upsert_uint32(pt, colnum, uint_fast32_t(value));
  case fptu_uint64:
    assert(is_within(value, 0u, UINT64_MAX));
    return fptu_upsert_uint64(pt, colnum, uint_fast64_t(value));
  case fptu_int32:
    assert(is_within(value, INT32_MIN, INT32_MAX));
    return fptu_upsert_int32(pt, colnum, int_fast32_t(value));
  case fptu_int64:
    assert(is_within(value, INT64_MIN, INT64_MAX));
    return fptu_upsert_int64(pt, colnum, int_fast64_t(value));
  case fptu_fp32:
    assert(value >= FLT_MIN && value <= FLT_MAX);
    return fptu_upsert_fp32(pt, colnum, float(value));
  case fptu_fp64:
    assert(value >= DBL_MIN && value <= DBL_MAX);
    return fptu_upsert_fp64(pt, colnum, double_t(value));
  }
}
#endif

int tuple2json(const fptu_ro &tuple, fptu_emit_func output, void *output_ctx,
               const string_view &indent, unsigned depth,
               const void *schema_ctx, fptu_tag2name_func tag2name,
               fptu_value2enum_func value2enum,
               const fptu_json_options options = fptu_json_default);

inline int tuple2json(const fptu_ro &tuple, FILE *file, const char *indent,
                      unsigned depth, const void *schema_ctx,
                      fptu_tag2name_func tag2name,
                      fptu_value2enum_func value2enum,
                      const fptu_json_options options = fptu_json_default) {

  return fptu_tuple2json_FILE(tuple, file, indent, depth, schema_ctx, tag2name,
                              value2enum, options);
}

/* Сериализует JSON-представление кортежа в std::ostream.
 *
 * Назначение параметров indent, depth, schema_ctx, tag2name и value2enum
 * см в описании функции fptu_tuple2json().
 *
 * Ошибки std::ostream обрабатываются в соответствии
 * с установками https://en.cppreference.com/w/cpp/io/basic_ios/exceptions,
 * если это не приводит к вбросу исключений, то возвращается -1.
 * При ошибках fptu возвращается соответствующий код ошибки.
 * При успешном завершении возвращается FPTU_SUCCESS (нуль). */
FPTU_API int tuple2json(const fptu_ro &tuple, std::ostream &stream,
                        const string_view &indent, unsigned depth,
                        const void *schema_ctx, fptu_tag2name_func tag2name,
                        fptu_value2enum_func value2enum,
                        const fptu_json_options options = fptu_json_default);

/* Сериализует JSON-представление кортежа в std::string и возвращает результат.
 *
 * Назначение параметров indent, depth, schema_ctx, tag2name и value2enum
 * см в описании функции fptu_tuple2json().
 *
 * При ошибках вбрасывает исключения, включая fptu::bad_tuple */
FPTU_API std::string
tuple2json(const fptu_ro &tuple, const string_view &indent, unsigned depth,
           const void *schema_ctx, fptu_tag2name_func tag2name,
           fptu_value2enum_func value2enum,
           const fptu_json_options options = fptu_json_default);

} // namespace fptu

//------------------------------------------------------------------------------

static __inline fptu_error fptu_upsert_string(fptu_rw *pt, unsigned column,
                                              const std::string &value) {
  return fptu_upsert_string(pt, column, value.data(), value.size());
}
static __inline fptu_error fptu_insert_string(fptu_rw *pt, unsigned column,
                                              const std::string &value) {
  return fptu_insert_string(pt, column, value.data(), value.size());
}

#if HAVE_cxx17_std_string_view
static __inline fptu_error fptu_upsert_string(fptu_rw *pt, unsigned column,
                                              const std::string_view &value) {
  return fptu_upsert_string(pt, column, value.data(), value.size());
}
static __inline fptu_error fptu_insert_string(fptu_rw *pt, unsigned column,
                                              const std::string_view &value) {
  return fptu_insert_string(pt, column, value.data(), value.size());
}
#endif /* HAVE_cxx17_std_string_view */

static __inline fptu_error fptu_upsert_string(fptu_rw *pt, unsigned column,
                                              const fptu::string_view &value) {
  return fptu_upsert_string(pt, column, value.data(), value.size());
}
static __inline fptu_error fptu_insert_string(fptu_rw *pt, unsigned column,
                                              const fptu::string_view &value) {
  return fptu_insert_string(pt, column, value.data(), value.size());
}

//------------------------------------------------------------------------------

namespace std {
FPTU_API string to_string(const fptu_error error);
FPTU_API string to_string(const fptu_type);
FPTU_API string to_string(const fptu_field &);
FPTU_API string to_string(const fptu_rw &);
FPTU_API string to_string(const fptu_ro &);
FPTU_API string to_string(const fptu_lge);
FPTU_API string to_string(const fptu::datetime_t &time);
} /* namespace std */

/* Явно удаляем лишенные смысла операции, в том числе для выявления ошибок */
bool operator>(const fptu_lge &, const fptu_lge &) = delete;
bool operator>=(const fptu_lge &, const fptu_lge &) = delete;
bool operator<(const fptu_lge &, const fptu_lge &) = delete;
bool operator<=(const fptu_lge &, const fptu_lge &) = delete;

#endif /* __cplusplus */

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* FAST_POSITIVE_TUPLES_H */
