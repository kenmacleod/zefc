#pragma once

#include "zefc/double_api.hpp"
#include "zefc/int_api.hpp"
#include "zefc/runtime.hpp"

namespace zefc {

// Per-call-site monomorphic method inline cache (Zef FunctionCallCache-style).
// Hit: isa_ guard matches → call cached zefc_method (skip slots[sel] walk).
// Miss: load slots[sel], prime guard + callee, invoke.
// Immediate Double/Int32 still short-circuit before any isa_ load.
//
// Sites must be unique per macro expansion (lambda local static), not
// template<__COUNTER__> — that ODR-merges across TUs and crosses selectors.

struct CallSite {
  int selector; // 0 = uninitialized (selector IDs start at 1)
  VTable* guard;
  zefc_method callee;
};

id zefc_call0_miss(id obj, CallSite* site);
id zefc_call1_miss(id obj, CallSite* site, id a1);
id zefc_call2_miss(id obj, CallSite* site, id a1, id a2);
id zefc_call3_miss(id obj, CallSite* site, id a1, id a2, id a3);
id zefc_call4_miss(id obj, CallSite* site, id a1, id a2, id a3, id a4);

inline void
call_site_ensure_sel(CallSite* site, int selector)
{
  if (site->selector == 0) {
    site->selector = selector;
  }
}

inline id
zefc_call0(id obj, CallSite* site)
{
  if (id_is_double(obj)) {
    return zefc_double_send0(obj, site->selector);
  }
  if (id_is_int32(obj)) {
    return zefc_int_send0(obj, site->selector);
  }
  if (site->guard == obj->isa_ && site->callee) {
    return site->callee(obj, site->selector);
  }
  return zefc_call0_miss(obj, site);
}

inline id
zefc_call1(id obj, CallSite* site, id a1)
{
  if (id_is_double(obj)) {
    return zefc_double_send1(obj, site->selector, a1);
  }
  if (id_is_int32(obj)) {
    return zefc_int_send1(obj, site->selector, a1);
  }
  if (site->guard == obj->isa_ && site->callee) {
    return site->callee(obj, site->selector, a1);
  }
  return zefc_call1_miss(obj, site, a1);
}

inline id
zefc_call2(id obj, CallSite* site, id a1, id a2)
{
  if (site->guard == obj->isa_ && site->callee) {
    return site->callee(obj, site->selector, a1, a2);
  }
  return zefc_call2_miss(obj, site, a1, a2);
}

inline id
zefc_call3(id obj, CallSite* site, id a1, id a2, id a3)
{
  if (site->guard == obj->isa_ && site->callee) {
    return site->callee(obj, site->selector, a1, a2, a3);
  }
  return zefc_call3_miss(obj, site, a1, a2, a3);
}

inline id
zefc_call4(id obj, CallSite* site, id a1, id a2, id a3, id a4)
{
  if (site->guard == obj->isa_ && site->callee) {
    return site->callee(obj, site->selector, a1, a2, a3, a4);
  }
  return zefc_call4_miss(obj, site, a1, a2, a3, a4);
}

} // namespace zefc
