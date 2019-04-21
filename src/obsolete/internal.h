static __inline bool match(const fptu_field *pf, unsigned column,
                           fptu_type_or_filter type_or_filter) {
  if (pf->colnum() != column)
    return false;
  if (is_filter(type_or_filter))
    return ((fptu_filter)type_or_filter & fptu_filter_mask(pf->type())) ? true
                                                                        : false;
  return (fptu_type)type_or_filter == pf->type();
}

static __inline std::size_t bytes2units(size_t bytes) {
  return (bytes + fptu_unit_size - 1) >> fptu_unit_shift;
}

static __inline std::size_t units2bytes(size_t units) {
  return units << fptu_unit_shift;
}

static __inline std::size_t tag_elem_size(fptu_tag_t tag) {
  fptu_type type = fptu_get_type(tag);
  if (likely(fptu_tag_is_fixedsize(type)))
    return fptu_internal_map_t2b[type];

  /* fptu_opaque, fptu_cstr or fptu_farray.
   * at least 4 bytes for length or '\0'. */
  return fptu_unit_size;
}

static __inline bool tag_match_fixedsize(fptu_tag_t tag, std::size_t units) {
  return fptu_tag_is_fixedsize(tag) &&
         units == fptu_internal_map_t2u[fptu_get_type(tag)];
}

size_t fptu_field_units(const fptu_field *pf);

static __inline const void *fptu_ro_detent(fptu_ro ro) {
  return (char *)ro.sys.iov_base + ro.sys.iov_len;
}

static __inline const void *fptu_detent(const fptu_rw *rw) {
  return &rw->units[rw->end];
}

fptu_field *fptu_lookup_tag(fptu_rw *pt, fptu_tag_t tag);
