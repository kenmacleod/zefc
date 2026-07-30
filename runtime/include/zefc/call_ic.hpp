#pragma once

#include "zefc/double_api.hpp"
#include "zefc/int_api.hpp"
#include "zefc/object_dispatch.hpp"
#include "zefc/runtime.hpp"

namespace zefc {

// Per-call-site method cache shared by object_dispatch=ic and =site.
//
// ic:   hit requires guard == obj->isa_; miss primes guard + callee via table.
// site: hit is unguarded sticky callee; first use (or call_site_bind) fills callee.
//
// Sites must be unique per macro expansion (statement-expr + __COUNTER__).

struct CallSite {
  int selector; // 0 = uninitialized (selector IDs start at 1)
  IsaPtr guard; // ic: class identity; site: unused on hit
  zefc_method callee;
};

id zefc_call0_miss(id obj, CallSite* site);
id zefc_call1_miss(id obj, CallSite* site, id a1);
id zefc_call2_miss(id obj, CallSite* site, id a1, id a2);
id zefc_call3_miss(id obj, CallSite* site, id a1, id a2, id a3);
id zefc_call4_miss(id obj, CallSite* site, id a1, id a2, id a3, id a4);

// Optional plan-B load-time bind: prime sticky callee from a known VTable.
void call_site_bind(CallSite* site, VTable* vt);

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

// site mode: sticky callee after first fill (plan B). Polymorphic call sites
// (e.g. richards) can call the wrong method — use ic/slots/flat for those, or
// call_site_bind() only at monomorphic closed-world sites.
inline id
zefc_site_call0(id obj, CallSite* site)
{
  if (id_is_double(obj)) {
    return zefc_double_send0(obj, site->selector);
  }
  if (id_is_int32(obj)) {
    return zefc_int_send0(obj, site->selector);
  }
  if (site->callee) {
    return site->callee(obj, site->selector);
  }
  return zefc_call0_miss(obj, site);
}

inline id
zefc_site_call1(id obj, CallSite* site, id a1)
{
  if (id_is_double(obj)) {
    return zefc_double_send1(obj, site->selector, a1);
  }
  if (id_is_int32(obj)) {
    return zefc_int_send1(obj, site->selector, a1);
  }
  if (site->callee) {
    return site->callee(obj, site->selector, a1);
  }
  return zefc_call1_miss(obj, site, a1);
}

inline id
zefc_site_call2(id obj, CallSite* site, id a1, id a2)
{
  if (site->callee) {
    return site->callee(obj, site->selector, a1, a2);
  }
  return zefc_call2_miss(obj, site, a1, a2);
}

inline id
zefc_site_call3(id obj, CallSite* site, id a1, id a2, id a3)
{
  if (site->callee) {
    return site->callee(obj, site->selector, a1, a2, a3);
  }
  return zefc_call3_miss(obj, site, a1, a2, a3);
}

inline id
zefc_site_call4(id obj, CallSite* site, id a1, id a2, id a3, id a4)
{
  if (site->callee) {
    return site->callee(obj, site->selector, a1, a2, a3, a4);
  }
  return zefc_call4_miss(obj, site, a1, a2, a3, a4);
}

} // namespace zefc
