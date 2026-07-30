#pragma once

#include "zefc/runtime.hpp"

#include <cstddef>
#include <cstdint>

namespace zefc {

// Per-call-site monomorphic field inline cache (Zef AccessCache-style).
//
// Preferred emission (transpile knows the field's class layout):
//   ZEFC_IC_GET(obj, sel, Body_, x) / ZEFC_IC_SET(..., val)
// Miss: validate registry, prime site->guard.
// Hit: if (site->guard) typed member load/store — unguarded, no call
//      (Zef UnguardedAccessCacheFeature). Fil-C-friendly vs getter calls.
//
// ZEFC_IC_*_OFFSET: type-erased byte-offset variant for polymorphic sites.

struct FieldSite {
  int selector;
  VTable* guard; // nullptr = empty / miss
  std::intptr_t offset; // for offset fallback; unused on typed hit
};

void field_register_get(VTable* vt, int selector, std::size_t offset);
void field_register_set(VTable* vt, int selector, std::size_t offset);

bool field_lookup_get(VTable* vt, int selector, std::intptr_t* out_offset);
bool field_lookup_set(VTable* vt, int selector, std::intptr_t* out_offset);

id zefc_ic_get_miss_send(id obj, FieldSite* site);
id zefc_ic_set_miss_send(id obj, FieldSite* site, id value);

inline id*
field_slot(id obj, std::intptr_t offset)
{
  return reinterpret_cast<id*>(reinterpret_cast<char*>(obj) + offset);
}

template<int Uid>
inline FieldSite*
field_site_uid(int selector)
{
  static FieldSite site{selector, nullptr, 0};
  return &site;
}

// --- Typed field IC (steady-state: guard + member access) ---

template<typename Body, id Body::*Member>
id zefc_ic_get_field_miss(id obj, FieldSite* site)
{
  if (!id_is_object(obj)) {
    return zefc_ic_get_miss_send(obj, site);
  }
  std::intptr_t off = 0;
  if (!field_lookup_get(obj->isa_, site->selector, &off)) {
    return zefc_ic_get_miss_send(obj, site);
  }
  site->guard = obj->isa_;
  site->offset = off;
  return body<Body>(obj)->*Member;
}

template<typename Body, id Body::*Member>
id zefc_ic_set_field_miss(id obj, FieldSite* site, id value)
{
  if (!id_is_object(obj)) {
    return zefc_ic_set_miss_send(obj, site, value);
  }
  std::intptr_t off = 0;
  if (!field_lookup_set(obj->isa_, site->selector, &off)) {
    return zefc_ic_set_miss_send(obj, site, value);
  }
  site->guard = obj->isa_;
  site->offset = off;
  body<Body>(obj)->*Member = value;
  return null_id();
}

template<typename Body, id Body::*Member>
inline id
zefc_ic_get_field(id obj, FieldSite* site)
{
  // After miss primes site->guard, steady-state is an unguarded typed load
  // (Zef UnguardedAccessCache-style). Miss path validates class + registration.
  if (site->guard) {
    return body<Body>(obj)->*Member;
  }
  return zefc_ic_get_field_miss<Body, Member>(obj, site);
}

template<typename Body, id Body::*Member>
inline id
zefc_ic_set_field(id obj, FieldSite* site, id value)
{
  if (site->guard) {
    body<Body>(obj)->*Member = value;
    return null_id();
  }
  return zefc_ic_set_field_miss<Body, Member>(obj, site, value);
}

// --- Type-erased offset IC (polymorphic / unknown layout at site) ---

id zefc_ic_get_offset_miss(id obj, FieldSite* site);
id zefc_ic_set_offset_miss(id obj, FieldSite* site, id value);

inline id
zefc_ic_get_offset(id obj, FieldSite* site)
{
  if (site->guard) {
    return *field_slot(obj, site->offset);
  }
  return zefc_ic_get_offset_miss(obj, site);
}

inline id
zefc_ic_set_offset(id obj, FieldSite* site, id value)
{
  if (site->guard) {
    *field_slot(obj, site->offset) = value;
    return null_id();
  }
  return zefc_ic_set_offset_miss(obj, site, value);
}

// Transpile-shaped default: typed member IC.
#define ZEFC_IC_GET(obj, sel_imm, BodyTy, member) \
  (zefc::zefc_ic_get_field<BodyTy, &BodyTy::member>( \
    (obj), zefc::field_site_uid<__COUNTER__>((sel_imm))))

#define ZEFC_IC_SET(obj, sel_imm, BodyTy, member, val) \
  (zefc::zefc_ic_set_field<BodyTy, &BodyTy::member>( \
    (obj), zefc::field_site_uid<__COUNTER__>((sel_imm)), (val)))

#define ZEFC_IC_GET_OFFSET(obj, sel_imm) \
  (zefc::zefc_ic_get_offset((obj), zefc::field_site_uid<__COUNTER__>((sel_imm))))

#define ZEFC_IC_SET_OFFSET(obj, sel_imm, val) \
  (zefc::zefc_ic_set_offset((obj), zefc::field_site_uid<__COUNTER__>((sel_imm)), (val)))

} // namespace zefc
