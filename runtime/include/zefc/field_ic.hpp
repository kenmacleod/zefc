#pragma once

#include "zefc/runtime.hpp"

namespace zefc {

// Per-call-site monomorphic field inline cache (Zef AccessCache-style).
// Hit: isa_ guard matches → call cached typed get/set (Fil-C-friendly).
// Miss: resolve from field registry; else full send.

using FieldGetter = id (*)(id self);
using FieldSetter = id (*)(id self, id value);

struct FieldSite {
  int selector;
  VTable* guard; // nullptr = empty
  FieldGetter getter;
  FieldSetter setter;
};

void field_register_get(VTable* vt, int selector, FieldGetter getter);
void field_register_set(VTable* vt, int selector, FieldSetter setter);

id zefc_ic_get_miss(id obj, FieldSite* site);
id zefc_ic_set_miss(id obj, FieldSite* site, id value);

// Unique static cell per call site (__COUNTER__). Avoids per-access lambda call.
template<int Uid>
inline FieldSite*
field_site_uid(int selector)
{
  static FieldSite site{selector, nullptr, nullptr, nullptr};
  return &site;
}

inline id
zefc_ic_get(id obj, FieldSite* site)
{
  if (id_is_object(obj) && site->guard == obj->isa_ && site->getter) {
    return site->getter(obj);
  }
  return zefc_ic_get_miss(obj, site);
}

inline id
zefc_ic_set(id obj, FieldSite* site, id value)
{
  if (id_is_object(obj) && site->guard == obj->isa_ && site->setter) {
    return site->setter(obj, value);
  }
  return zefc_ic_set_miss(obj, site, value);
}

#define ZEFC_IC_GET(obj, sel_imm) \
  (zefc::zefc_ic_get((obj), zefc::field_site_uid<__COUNTER__>((sel_imm))))

#define ZEFC_IC_SET(obj, sel_imm, val) \
  (zefc::zefc_ic_set((obj), zefc::field_site_uid<__COUNTER__>((sel_imm)), (val)))

} // namespace zefc
