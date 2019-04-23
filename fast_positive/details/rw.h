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
#include "fast_positive/details/1Hippeus/buffer.h"
#include "fast_positive/details/api.h"
#include "fast_positive/details/essentials.h"
#include "fast_positive/details/exceptions.h"
#include "fast_positive/details/field.h"
#include "fast_positive/details/getter.h"
#include "fast_positive/details/meta.h"
#include "fast_positive/details/ro.h"
#include "fast_positive/details/scan.h"
#include "fast_positive/details/schema.h"
#include "fast_positive/details/token.h"

#include <algorithm> // for std::min/max

#include "fast_positive/details/warnings_push_pt.h"

namespace fptu {

class tuple_rw_managed;

namespace details {

class FPTU_API tuple_rw : public crtp_getter<tuple_rw> {
  friend class crtp_getter<tuple_rw>;
  friend class fptu::tuple_rw_managed;

public:
  /* closed and noncopyable. */
  tuple_rw(const tuple_rw &) = delete;
  tuple_rw &operator=(tuple_rw const &) = delete;

  static void *operator new[](std::size_t object_size) = delete;
  static void operator delete[](void *ptr) = delete;

  struct deleter {
    void operator()(/* accepts nullptr */ tuple_rw *ptr) {
      if (ptr)
        ptr->~tuple_rw();
    }
  };

  static tuple_rw *create_new(std::size_t items_limit, std::size_t data_bytes,
                              const fptu::schema *schema,
                              const hippeus::buffer_tag &allot_tag =
                                  hippeus::buffer_tag(&hippeus_allot_stdcxx,
                                                      false));

  static tuple_rw *
  create_from_ro(const audit_holes_info &holes_info, const tuple_ro *ro,
                 std::size_t more_items, std::size_t more_payload,
                 const fptu::schema *schema,
                 const hippeus::buffer_tag &allot_tag =
                     hippeus::buffer_tag(&hippeus_allot_stdcxx, false));

  static tuple_rw *
  create_from_buffer(const void *source_ptr, std::size_t source_bytes,
                     std::size_t more_items, std::size_t more_payload,
                     const fptu::schema *schema,
                     const hippeus::buffer_tag &allot_tag =
                         hippeus::buffer_tag(&hippeus_allot_stdcxx, false));

  static tuple_rw *create_legacy(void *buffer_space, std::size_t buffer_size,
                                 std::size_t items_limit);
  static tuple_rw *fetch_legacy(const audit_holes_info &holes_info,
                                const tuple_ro *ro, void *buffer_space,
                                std::size_t buffer_size,
                                std::size_t more_items);

  const hippeus::buffer *get_buffer() const noexcept {
    return const_cast<tuple_rw *>(this)->get_buffer();
  }

protected:
  inline tuple_rw(int buffer_offset, std::size_t buffer_size,
                  std::size_t items_limit, const fptu::schema *schema);
  inline tuple_rw(const audit_holes_info &holes_info, const tuple_ro *ro,
                  int buffer_offset, std::size_t buffer_size,
                  std::size_t more_items, const fptu::schema *schema);

  const fptu::schema *schema_;

  int buffer_offset_;
  static int get_buffer_offset(hippeus::buffer *buffer) noexcept {
    const ptrdiff_t full_offset =
        erthink::constexpr_pointer_cast<char *>(buffer) -
        erthink::constexpr_pointer_cast<char *>(buffer->data());
    assert(full_offset != 0);
    assert(full_offset > INT_MIN && full_offset < INT_MAX);
    return int(full_offset);
  }
  hippeus::buffer *get_buffer() noexcept {
    return buffer_offset_ ? erthink::constexpr_pointer_cast<hippeus::buffer *>(
                                erthink::constexpr_pointer_cast<char *>(this) +
                                buffer_offset_)
                          : nullptr;
  }
  ~tuple_rw();
  unsigned options_;

  union FPTU_API junk_counters {
    /* Счетчики мусорных 32-битных элементов, которые образовались при
     * удалении/обновлении. */
    struct {
      uint16_t count, volume;
    };
    uint32_t both;
    constexpr inline junk_counters() : both(0) {
      static_assert(sizeof(junk_counters) == 4, "WTF?");
    }
    constexpr inline junk_counters(const struct audit_holes_info &holes_info)
        : count(uint16_t(holes_info.count)),
          volume(uint16_t(holes_info.volume)) {}
  };
  junk_counters junk_;

  unsigned end_ /* Индекс после конца выделенного буфера */;
  unsigned pivot_ /* Индекс опорной точки, от которой растут "голова" и
                     "хвоcт", указывает на терминатор заголовка. */
      ;
  unsigned head_ /* Индекс дозаписи дескрипторов, растет к началу буфера,
                    указывает на первый занятый элемент. */
      ;
  unsigned tail_ /* Индекс для дозаписи данных, растет к концу буфера,
                    указываент на первый не занятый элемент. */
      ;
  unit_t reserve_for_RO_header;
  unit_t area_[1];

  void *pivot() noexcept { return &area_[pivot_]; }
  const void *pivot() const noexcept {
    return const_cast<tuple_rw *>(this)->pivot();
  }

  field_loose *begin_index() noexcept {
    return erthink::constexpr_pointer_cast<field_loose *>(&area_[head_]);
  }
  field_loose *end_index() noexcept {
    return static_cast<field_loose *>(pivot());
  }

  const field_loose *begin_index() const noexcept {
    return const_cast<tuple_rw *>(this)->begin_index();
  }
  const field_loose *end_index() const noexcept {
    return const_cast<tuple_rw *>(this)->end_index();
  }

  const unit_t *begin_data_units() const noexcept {
    return static_cast<const unit_t *>(pivot());
  }
  const unit_t *end_data_units() const noexcept { return &area_[tail_]; }
  const char *begin_data_bytes() const noexcept {
    return erthink::constexpr_pointer_cast<const char *>(begin_data_units());
  }
  const char *end_data_bytes() const noexcept {
    return erthink::constexpr_pointer_cast<const char *>(end_data_units());
  }

  relative_payload *alloc_data(const std::size_t units);
  void release_data(relative_payload *chunk, const std::size_t units);
  field_loose *alloc_loose(const tag_t tag, const std::size_t units);
  void release_loose(field_loose *loose, const std::size_t units);
  relative_payload *realloc_data(relative_offset &ref, const std::size_t have,
                                 const std::size_t needed);
  static const char *audit(const tuple_rw *self) noexcept;
  //----------------------------------------------------------------------------
  template <
      typename TOKEN, genus GENUS,
      typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique !=
                                    meta::physique_kind::stretchy> * = nullptr>
  inline combo_ptr assign_field_value(
      combo_ptr field,
      const typename meta::genus_traits<GENUS>::return_type value,
      const TOKEN token);

  template <
      typename TOKEN, genus GENUS,
      typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique ==
                                    meta::physique_kind::stretchy> * = nullptr>
  inline combo_ptr assign_field_value(
      combo_ptr field,
      const typename meta::genus_traits<GENUS>::return_type value,
      const TOKEN token);

  template <
      typename TOKEN, genus GENUS,
      typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique !=
                                    meta::physique_kind::stretchy> * = nullptr>
  inline field_loose *
  append_field(const typename meta::genus_traits<GENUS>::return_type value,
               const TOKEN token);

  template <
      typename TOKEN, genus GENUS,
      typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique ==
                                    meta::physique_kind::stretchy> * = nullptr>
  inline field_loose *
  append_field(const typename meta::genus_traits<GENUS>::return_type value,
               const TOKEN token);

  void erase(field_preplaced *preplaced, const genus type,
             const bool distinct_null) {
    debug_check();
    if (is_fixed_size(type)) {
      preplaced_erase(type, preplaced, distinct_null);
    } else if (preplaced->relative.have_payload()) {
      relative_payload *const payload = preplaced->relative.payload();
      release_data(payload, payload->stretchy.brutto_units(type));
      preplaced->relative.reset_payload();
    }
    debug_check();
  }

public:
  template <typename> class accessor_rw;
  template <typename> class iterator_rw;
  template <typename> class collection_rw;

  void erase(field_loose *loose) {
    debug_check();
    assert(loose && loose != end_index());
    assert(!loose->is_hole());
    const genus type = loose->type();
    const std::size_t space =
        is_fixed_size(type) ? loose_units(type) : loose->stretchy_units();
    release_loose(loose, space);
    debug_check();
  }

  template <typename TOKEN> class accessor_rw : public accessor_ro<TOKEN> {
    using base = accessor_ro<TOKEN>;
    template <typename> friend class crtp_getter;
    friend class tuple_rw;
    tuple_rw *tuple_;

    template <genus GENUS>
    inline void
    assign(const typename meta::genus_traits<GENUS>::return_type value);

    inline void remove();

    constexpr accessor_rw() : base(), tuple_(nullptr) {
      static_assert(sizeof(*this) <= (TOKEN::is_static_token::value
                                          ? sizeof(void *) * 2
                                          : sizeof(void *) * 3),
                    "WTF?");
    }

    constexpr accessor_rw(tuple_rw *tuple, field_preplaced *target,
                          const TOKEN token) noexcept
        : base(target, token), tuple_(tuple) {}

    constexpr accessor_rw(tuple_rw *tuple, field_loose *target,
                          const TOKEN token) noexcept
        : base(target, tuple->end_index(), token), tuple_(tuple) {}

    constexpr accessor_rw(tuple_rw *tuple, const base &accessor) noexcept
        : base(accessor), tuple_(tuple) {}

  public:
    constexpr accessor_rw(const accessor_rw &) noexcept = default;
    cxx14_constexpr accessor_rw &
    operator=(const accessor_rw &) noexcept = default;
    constexpr accessor_rw(accessor_rw &&) noexcept = default;

    constexpr tuple_rw *tuple() const noexcept { return tuple_; }

    //--------------------------------------------------------------------------

    void set_string(const string_view &value) { return assign<text>(value); }
    void set_varbinary(const string_view &value) {
      return assign<varbin>(value);
    }
    void set_nested(const tuple_ro *value) {
      static_assert(std::is_base_of<stretchy_value_tuple, tuple_ro>::value,
                    "WTF?");
      return assign<nested>(
          erthink::constexpr_pointer_cast<const stretchy_value_tuple *>(value));
    }
    void set_property(const property_pair &value) {
      return assign<property>(value);
    }
    void set_bool(const bool value) { return assign<boolean>(value); }
    void set_enum(const short value) { return assign<enumeration>(value); }
    void set_i8(const int8_t value) { return assign<i8>(value); }
    void set_u8(const uint8_t value) { return assign<u8>(value); }
    void set_i16(const int16_t value) { return assign<i16>(value); }
    void set_u16(const uint16_t value) { return assign<u16>(value); }
    void set_i32(const int32_t value) { return assign<i32>(value); }
    void set_u32(const uint32_t value) { return assign<u32>(value); }
    void set_i64(const int64_t value) { return assign<i64>(value); }
    void set_u64(const uint64_t value) { return assign<u64>(value); }
    void set_f32(const float value) { return assign<f32>(value); }
    void set_f64(const double value) { return assign<f64>(value); }
    void set_decimal(const decimal64 value) { return assign<d64>(value); }
    void set_datetime(const datetime_t value) { return assign<t64>(value); }
    void set_uuid(const uuid_t &value) { return assign<b128>(value.bin128); }

    void set_b96(const binary96_t &value) { return assign<b96>(value); }
    void set_b128(const binary128_t &value) { return assign<b128>(value); }
    void set_b160(const binary160_t &value) { return assign<b160>(value); }
    void set_b192(const binary192_t &value) { return assign<b192>(value); }
    void set_b224(const binary224_t &value) { return assign<b224>(value); }
    void set_b256(const binary256_t &value) { return assign<b256>(value); }
    void set_b320(const binary320_t &value) { return assign<b320>(value); }
    void set_b384(const binary384_t &value) { return assign<b384>(value); }
    void set_b512(const binary512_t &value) { return assign<b512>(value); }

    void set_ip_address(const ip_address_t &value) { return assign<ip>(value); }
    void set_mac_address(const mac_address_t value) {
      return assign<mac>(value);
    }
    void set_ip_net(const ip_net_t &value) { return assign<ipnet>(value); }

    void set_float(const double value) {
      if (base::type() == f32) {
        if (!std::isinf(value) &&
            unlikely(value > std::numeric_limits<float>::max() ||
                     value < std::numeric_limits<float>::lowest()))
          throw_value_range();
        set_f32(static_cast<float>(value));
      } else {
        set_f64(value);
      }
    }

    void set_integer(const int64_t value) {
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case i8:
        if (unlikely(value != static_cast<int8_t>(value)))
          throw_value_range();
        return set_i8(static_cast<int8_t>(value));
      case i16:
        if (unlikely(value != static_cast<int16_t>(value)))
          throw_value_range();
        return set_i16(static_cast<int16_t>(value));
      case i32:
        if (unlikely(value != static_cast<int32_t>(value)))
          throw_value_range();
        return set_i32(static_cast<int32_t>(value));
      case i64:
        return set_i64(value);
      }
    }

    void set_integer(const uint64_t value) {
      if (unlikely(value > uint64_t(std::numeric_limits<int64_t>::max())))
        throw_value_range();
      set_integer(static_cast<int64_t>(value));
    }

    void set_unsigned(const uint64_t value) {
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case u8:
        if (unlikely(value != static_cast<uint8_t>(value)))
          throw_value_range();
        return set_u8(static_cast<uint8_t>(value));
      case u16:
        if (unlikely(value != static_cast<uint16_t>(value)))
          throw_value_range();
        return set_u16(static_cast<uint16_t>(value));
      case u32:
        if (unlikely(value != static_cast<uint32_t>(value)))
          throw_value_range();
        return set_u32(static_cast<uint32_t>(value));
      case u64:
        return set_u64(value);
      }
    }

    void set_unsigned(const int64_t value) {
      if (unlikely(value < 0))
        throw_value_range();
      set_integer(static_cast<uint64_t>(value));
    }

    void set_int128(const int128_t &value) {
      if (base::type() == b128)
        return assign<b128>(
            *erthink::constexpr_pointer_cast<const binary128_t *>(&value));
      set_integer(int64_t(value));
    }

    void set_uint128(const uint128_t &value) {
      if (base::type() == b128)
        return assign<b128>(
            *erthink::constexpr_pointer_cast<const binary128_t *>(&value));
      set_unsigned(uint64_t(value));
    }
  };

  //----------------------------------------------------------------------------

  template <typename TOKEN>
  class iterator_rw
      : protected accessor_rw<TOKEN>
#if __cplusplus < 201703L
      ,
        public std::iterator<
            std::forward_iterator_tag, const accessor_rw<TOKEN>, std::ptrdiff_t,
            const accessor_rw<TOKEN> *, const accessor_rw<TOKEN> &>
#endif
  {
    friend class tuple_rw;
    using accessor = accessor_rw<TOKEN>;

    explicit constexpr iterator_rw(tuple_rw *tuple, field_loose *target,
                                   const TOKEN token) noexcept
        : accessor(tuple, target, token) {
      constexpr_assert(token.is_collection());
    }

  public:
#if __cplusplus >= 201703L
    using iterator_category = std::forward_iterator_tag;
    using value_type = const accessor_rw<TOKEN>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;
#endif

    constexpr iterator_rw() noexcept : accessor() {
      static_assert(sizeof(*this) <= (TOKEN::is_static_token::value
                                          ? sizeof(void *) * 2
                                          : sizeof(void *) * 3),
                    "WTF?");
    }
    constexpr iterator_rw(const iterator_rw &) noexcept = default;
    cxx14_constexpr iterator_rw &
    operator=(const iterator_rw &) noexcept = default;
    constexpr const TOKEN &token() const noexcept { return accessor::token(); }

    iterator_rw &operator++() noexcept {
      assert(accessor::field_.loose < accessor::tuple_->end_index());
      accessor::field_.const_loose =
          next(accessor::field_.const_loose, accessor::tuple_->end_index(),
               accessor::token_.tag());
      return *this;
    }
    iterator_rw operator++(int) const noexcept {
      iterator_rw iterator(*this);
      ++iterator;
      return iterator;
    }
    cxx14_constexpr accessor operator*() const noexcept {
      return accessor(*this);
    }
    cxx14_constexpr accessor &operator*() noexcept { return *this; }

    constexpr bool operator==(const accessor &other) const noexcept {
      return accessor::field_.ptr == other.field_.ptr;
    }
    constexpr bool operator!=(const accessor &other) const noexcept {
      return accessor::field_.ptr != other.field_.ptr;
    }
    constexpr bool operator==(const iterator_rw &other) const noexcept {
      return accessor::field_.ptr == other.field_.ptr;
    }
    constexpr bool operator!=(const iterator_rw &other) const noexcept {
      return accessor::field_.ptr != other.field_.ptr;
    }

    operator iterator_ro<TOKEN>() const noexcept {
      return iterator_ro<TOKEN>(accessor::field_.const_loose,
                                accessor::tuple_->end_index(),
                                accessor::token_);
    }
    constexpr bool operator==(const iterator_ro<TOKEN> &other) const noexcept {
      return accessor::field_.ptr == other.field_.ptr;
    }
    constexpr bool operator!=(const iterator_ro<TOKEN> &other) const noexcept {
      return accessor::field_.ptr != other.field_.ptr;
    }
  };

  template <typename TOKEN> class collection_rw : protected iterator_rw<TOKEN> {
    template <typename> friend class crtp_getter;
    friend class tuple_rw;

    constexpr collection_rw(tuple_rw *tuple, field_loose *first,
                            const TOKEN token) noexcept
        : iterator_rw<TOKEN>(tuple, first, token) {
      constexpr_assert(token.is_collection());
    }

  public:
    using const_iterator = iterator_ro<TOKEN>;
    using const_range = collection_ro<TOKEN>;
    using iterator = iterator_rw<TOKEN>;
    constexpr collection_rw(const collection_rw &) = default;
    cxx14_constexpr collection_rw &operator=(const collection_rw &) = default;
    constexpr const TOKEN &token() const noexcept { return iterator::token(); }

    constexpr operator const_range() const noexcept {
      return const_range(iterator::field_.const_loose,
                         iterator::tuple_->end_index(), iterator::token_);
    }

    cxx14_constexpr const const_iterator begin() const noexcept {
      return const_iterator(iterator::field_.const_loose,
                            iterator::tuple_->end_index(), iterator::token_);
    }
    cxx14_constexpr const_iterator end() const noexcept {
      return const_iterator(nullptr, iterator::tuple_->end_index(),
                            iterator::token_);
    }

    cxx14_constexpr const iterator &begin() noexcept { return *this; }
    cxx14_constexpr iterator end() noexcept {
      return iterator(iterator::tuple_, nullptr, iterator::token_);
    }

    constexpr bool empty() const noexcept { return begin() == end(); }
  };

private:
  template <typename TOKEN>
  __pure_function accessor_rw<TOKEN> at(const TOKEN &token) {
    return accessor_rw<TOKEN>(this, crtp_getter::at(*this, token));
  }

  template <typename TOKEN>
  __pure_function collection_rw<TOKEN> collect(const TOKEN &token) {
    if (unlikely(!token.is_collection()))
      throw_collection_required();

    field_loose *const first = const_cast<field_loose *>(
        lookup(is_sorted(), begin_index(), end_index(), token.tag()));
    return collection_rw<TOKEN>(this, first, token);
  }

  template <typename TOKEN>
  __pure_function accessor_rw<TOKEN> append(const TOKEN &token) {
    if (unlikely(!token.is_collection()))
      throw_collection_required();

    return accessor_rw<TOKEN>(this, static_cast<field_loose *>(nullptr), token);
  }

  template <typename TOKEN> __pure_function bool remove(const TOKEN &token) {
    accessor_rw<TOKEN> accessor(
        this, crtp_getter::template at<TOKEN, true>(*this, token));
    if (accessor.exist()) {
      accessor.remove();
      return true;
    }
    return false;
  }

public: //----------------------------------------------------------------------
  void ensure();
  void debug_check() {
#ifndef NDEBUG
    ensure();
#endif
  }
  void reset() noexcept;

  enum class optimize_flags {
    none = 0,
    compactify = 1,
    sort_index = 2,
    force_sort_index = 4,
    all = optimize_flags::compactify | optimize_flags::sort_index,
  };
  friend inline optimize_flags operator|(optimize_flags a, optimize_flags b) {
    return optimize_flags(unsigned(a) | unsigned(b));
  }
  friend inline bool operator&(optimize_flags a, optimize_flags b) {
    return (unsigned(a) & unsigned(b)) != 0;
  }

  bool optimize(const optimize_flags flags = optimize_flags::all);
  bool compactify() { return optimize(optimize_flags::compactify); }
  bool sort_index(bool force = false) {
    return optimize(force ? optimize_flags::force_sort_index
                          : optimize_flags::sort_index);
  }

  bool erase(const iterator_rw<token> &it);
  std::size_t erase(const iterator_rw<token> &begin,
                    const iterator_rw<token> &end);
  std::size_t erase(const collection_rw<token> &collection);
  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  bool erase(const TOKEN &ident) {
    return likely(!ident.is_collection()) ? remove(ident)
                                          : erase(collection(ident)) > 0;
  }
  bool erase(const token &ident);
  const char *audit() const noexcept { return audit(this); }

  std::size_t head_space() const noexcept { return head_; }
  std::size_t tail_space() const noexcept {
    assert(end_ >= tail_);
    return units2bytes(end_ - tail_);
  }
  std::size_t junk_space() const noexcept {
    return units2bytes(junk_.volume + junk_.count);
  }
  bool empty() const noexcept { return head_ + junk_.count == tail_; }
  std::size_t index_size() const noexcept {
    assert(pivot_ >= head_);
    return pivot_ - head_;
  }
  std::size_t loose_count() const noexcept {
    assert(index_size() >= junk_.count);
    return index_size() - junk_.count;
  }
  std::size_t brutto_size() const noexcept {
    assert(end_ >= tail_);
    return units2bytes(tail_ - end_ + /* header */ 1);
  }
  std::size_t netto_size() const noexcept {
    return brutto_size() - junk_space();
  }
  bool have_preplaced() const noexcept {
    return schema_ && schema_->preplaced_bytes() > 0;
  }
  bool is_sorted() const noexcept {
    /* TODO */
    return false;
  }

  static constexpr std::size_t pure_tuple_size() {
    return sizeof(tuple_rw) - sizeof(tuple_rw::area_);
  }

  static size_t estimate_required_space(size_t items, std::size_t data_bytes,
                                        const fptu::schema *schema,
                                        bool dont_account_preplaced = false) {
    const std::size_t preplaced_bytes = schema ? schema->preplaced_bytes() : 0;
    if (unlikely(preplaced_bytes > max_tuple_bytes))
      throw_invalid_schema();
    if (unlikely(data_bytes > max_tuple_bytes - preplaced_bytes ||
                 items > max_fields))
      throw_tuple_too_large();

    return pure_tuple_size() + items * unit_size +
           utils::ceil(dont_account_preplaced ? data_bytes
                                              : data_bytes + preplaced_bytes,
                       unit_size);
  }

  static std::size_t estimate_required_space(const tuple_ro *ro,
                                             const std::size_t more_items,
                                             const std::size_t more_payload,
                                             const fptu::schema *schema) {
    /* Во избежание двойной работы здесь НЕ проверяется корректность переданного
     * экземпляра tuple_ro и его соответствие схеме. Такая проверка должна
     * выполняться один раз явным вызовом валидирующих функций. Например, внутри
     * конструкторов безопасных объектов. */
    if (unlikely(more_items > max_fields))
      throw_invalid_argument("items > fptu::max_fields");
    if (unlikely(more_payload > fptu::buffer_limit))
      throw_invalid_argument("more_payload_bytes > fptu::buffer_limit");

    const std::size_t total_items =
        std::min(size_t(fptu::max_fields), ro->index_size() + more_items);

    const std::size_t total_payload = std::min(
        size_t(fptu::max_tuple_bytes), ro->payload_bytes() + more_payload);

    return estimate_required_space(total_items, total_payload, schema,
                                   /* не учитываем размер preplaced-полей,
                                    * ибо они уже должы быть в tuple_ro */
                                   true);
  }

  bool operator==(const tuple_ro *ro) const noexcept {
    return ro->flat == &area_[head_ - 1];
  }
  bool operator!=(const tuple_ro *ro) const noexcept { return !(*this == ro); }

  const tuple_ro *take_asis() const;
  std::pair<const tuple_ro *, bool> take_optimized();

#define HERE_GENUS_CASE(VALUE_TYPE, NAME)                                      \
  FPTU_TEMPLATE_FOR_STATIC_TOKEN                                               \
  inline void set_##NAME(const TOKEN &ident, const VALUE_TYPE value) {         \
    at(ident).set_##NAME(value);                                               \
  }                                                                            \
  inline void legacy_update_##NAME(const token &ident,                         \
                                   const VALUE_TYPE value) {                   \
    accessor_rw<token> accessor(                                               \
        this, crtp_getter::template at<token, true>(*this, ident));            \
    if (unlikely(!accessor.exist()))                                           \
      throw_field_absent();                                                    \
    accessor.set_##NAME(value);                                                \
  }                                                                            \
  void set_##NAME(const token &ident, const VALUE_TYPE value);                 \
  iterator_rw<token> insert_##NAME(const token &ident, const VALUE_TYPE value);
  HERE_GENUS_CASE(tuple_ro *, nested)
  HERE_GENUS_CASE(string_view &, string)
  HERE_GENUS_CASE(string_view &, varbinary)
  HERE_GENUS_CASE(property_pair &, property)
  HERE_GENUS_CASE(bool, bool)
  HERE_GENUS_CASE(short, enum)
  HERE_GENUS_CASE(int8_t, i8)
  HERE_GENUS_CASE(uint8_t, u8)
  HERE_GENUS_CASE(int16_t, i16)
  HERE_GENUS_CASE(uint16_t, u16)
  HERE_GENUS_CASE(int32_t, i32)
  HERE_GENUS_CASE(uint32_t, u32)
  HERE_GENUS_CASE(int64_t, i64)
  HERE_GENUS_CASE(uint64_t, u64)
  HERE_GENUS_CASE(float, f32)
  HERE_GENUS_CASE(double, f64)
  HERE_GENUS_CASE(decimal64, decimal)
  HERE_GENUS_CASE(datetime_t, datetime)
  HERE_GENUS_CASE(uuid_t &, uuid)
  HERE_GENUS_CASE(int128_t &, int128)
  HERE_GENUS_CASE(uint128_t &, uint128)
  HERE_GENUS_CASE(binary96_t &, b96)
  HERE_GENUS_CASE(binary128_t &, b128)
  HERE_GENUS_CASE(binary160_t &, b160)
  HERE_GENUS_CASE(binary192_t &, b192)
  HERE_GENUS_CASE(binary224_t &, b224)
  HERE_GENUS_CASE(binary256_t &, b256)
  HERE_GENUS_CASE(binary320_t &, b320)
  HERE_GENUS_CASE(binary384_t &, b384)
  HERE_GENUS_CASE(binary512_t &, b512)
  HERE_GENUS_CASE(ip_address_t &, ip_address)
  HERE_GENUS_CASE(mac_address_t, mac_address)
  HERE_GENUS_CASE(ip_net_t &, ip_net)
  HERE_GENUS_CASE(int64_t, integer)
  HERE_GENUS_CASE(uint64_t, integer)
  HERE_GENUS_CASE(int64_t, unsigned)
  HERE_GENUS_CASE(uint64_t, unsigned)
  HERE_GENUS_CASE(double, float)
#undef HERE_GENUS_CASE

  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  __pure_function inline collection_rw<TOKEN> collection(const TOKEN &ident) {
    return collect(ident);
  }
  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  __pure_function inline accessor_rw<TOKEN> operator[](const TOKEN &ident) {
    return at(ident);
  }
  __pure_function collection_rw<token> collection(const token &ident);
  __pure_function accessor_rw<token> operator[](const token &ident);

  template <typename TOKEN>
  __pure_function inline bool is_present(const TOKEN &ident) const {
    return crtp_getter::is_present(ident);
  }
};

//------------------------------------------------------------------------------

template <typename TOKEN, genus GENUS,
          typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique !=
                                        meta::physique_kind::stretchy> *>
inline combo_ptr tuple_rw::assign_field_value(
    combo_ptr field,
    const typename meta::genus_traits<GENUS>::return_type value,
    const TOKEN token) {
  if (unlikely(token.type() != GENUS))
    throw_type_mismatch();

  debug_check();
  if (likely(token.is_preplaced())) {
    // preplaced fixed-size field
    if (token.distinct_null() &&
        unlikely(meta::genus_traits<GENUS>::is_prohibited_nil(value)))
      throw_value_denil();
    // write value, no exceptions allowed
    meta::genus_traits<GENUS>::write(field.preplaced, value);
  } else {
    // loose
    if (field.loose == nullptr) {
      // absent, append new one (exception may be thrown)
      field.loose =
          alloc_loose(token.tag(), meta::genus_traits<GENUS>::loose_units);
    }
    if (unlikely(field.loose >= end_index() || field.loose->type() != GENUS))
      throw_index_corrupted();
    // write value, no exceptions allowed
    meta::genus_traits<GENUS>::write(field.loose, value);
  }
  debug_check();
  return field;
}

template <typename TOKEN, genus GENUS,
          typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique ==
                                        meta::physique_kind::stretchy> *>
inline combo_ptr tuple_rw::assign_field_value(
    combo_ptr field,
    const typename meta::genus_traits<GENUS>::return_type value,
    const TOKEN token) {
  if (unlikely(token.type() != GENUS))
    throw_type_mismatch();

  debug_check();
  const std::size_t needed =
      (token.distinct_null() || !meta::genus_traits<GENUS>::is_empty(value))
          ? meta::genus_traits<GENUS>::estimate_space(value)
          : 0;
  if (token.is_loose()) {
    if (field.loose == nullptr) {
      // absent loose, append new one (exception may be thrown)
      field.loose = alloc_loose(token.tag(), needed);
      if (needed) {
        assert(tag2genus(field.loose->genius_and_id) == GENUS);
        // write value, no exceptions allowed
        meta::genus_traits<GENUS>::write(field.loose->relative.payload(),
                                         value);
      }
      debug_check();
      return field;
    }
    if (unlikely(field.loose >= end_index() || field.loose->type() != GENUS))
      throw_index_corrupted();
  }

  auto &relative = field.relative_reference();
  relative_payload *payload = nullptr;
  if (relative.have_payload()) {
    payload = relative.payload();
    const std::size_t have = payload->stretchy.brutto_units(GENUS);
    if (unlikely(needed != have)) {
      if (needed == 0) {
        // exception may be thrown in case no index-space
        release_data(payload, have);
        relative.reset_payload();
        debug_check();
        return field;
      }

      // exception may be thrown
      payload = realloc_data(relative, have, needed);
      assert(relative.payload() == payload);
    }
  } else {
    if (needed == 0) {
      debug_check();
      return field;
    }
    // exception may be thrown
    payload = alloc_data(needed);
    // update offset, no exceptions allowed
    relative.set_payload(payload);
  }

  assert(needed > 0 && payload);
  // write value, no exceptions allowed
  meta::genus_traits<GENUS>::write(payload, value);
  debug_check();
  return field;
}

//------------------------------------------------------------------------------

template <typename TOKEN, genus GENUS,
          typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique !=
                                        meta::physique_kind::stretchy> *>
inline field_loose *tuple_rw::append_field(
    const typename meta::genus_traits<GENUS>::return_type value,
    const TOKEN token) {
  if (unlikely(token.type() != GENUS))
    throw_type_mismatch();
  if (unlikely(!token.is_collection()))
    throw_collection_required();

  debug_check();
  // append new one (exception may be thrown)
  field_loose *field =
      alloc_loose(token.tag(), meta::genus_traits<GENUS>::loose_units);
  assert(tag2genus(field->genius_and_id) == GENUS);

  // write value, no exceptions allowed
  meta::genus_traits<GENUS>::write(field, value);
  debug_check();
  return field;
}

template <typename TOKEN, genus GENUS,
          typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique ==
                                        meta::physique_kind::stretchy> *>
inline field_loose *tuple_rw::append_field(
    const typename meta::genus_traits<GENUS>::return_type value,
    const TOKEN token) {
  if (unlikely(token.type() != GENUS))
    throw_type_mismatch();
  if (unlikely(!token.is_collection()))
    throw_collection_required();

  debug_check();
  const std::size_t needed =
      (token.distinct_null() || !meta::genus_traits<GENUS>::is_empty(value))
          ? meta::genus_traits<GENUS>::estimate_space(value)
          : 0;
  // append new one (exception may be thrown)
  field_loose *field = alloc_loose(token.tag(), needed);
  if (needed) {
    assert(tag2genus(field->genius_and_id) == GENUS);
    // write value, no exceptions allowed
    meta::genus_traits<GENUS>::write(field->relative.payload(), value);
  } else {
    assert(!field->relative.have_payload());
  }
  debug_check();
  return field;
}

//------------------------------------------------------------------------------

template <typename TOKEN>
template <genus GENUS>
inline void tuple_rw::accessor_rw<TOKEN>::assign(
    const typename meta::genus_traits<GENUS>::return_type value) {
  base::field_ = tuple_->template assign_field_value<TOKEN, GENUS>(
      base::field_, value, base::token_);
}

template <typename TOKEN> inline void tuple_rw::accessor_rw<TOKEN>::remove() {
  if (likely(base::token_.is_preplaced())) {
    tuple_->erase(base::field_.preplaced, base::token_.type(),
                  base::token_.distinct_null());
  } else {
    assert(base::field_.loose != nullptr);
    if (unlikely(base::field_.loose >= tuple_->end_index() ||
                 base::field_.loose->type() != base::token_.type()))
      throw_index_corrupted();
    tuple_->erase(base::field_.loose);
    base::field_.loose = nullptr;
  }
}

} // namespace details

//------------------------------------------------------------------------------

using dynamic_accessor_rw = details::tuple_rw::accessor_rw<token>;
using dynamic_iterator_rw = details::tuple_rw::iterator_rw<token>;
using dynamic_collection_rw = details::tuple_rw::collection_rw<token>;

} // namespace fptu

#include "fast_positive/details/warnings_pop.h"
