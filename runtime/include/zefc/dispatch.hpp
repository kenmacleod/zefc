#pragma once

#include "zefc/double_api.hpp"
#include "zefc/field_ic.hpp"
#include "zefc/int_api.hpp"
#include "zefc/runtime.hpp"

// Method dispatch for fixed-site ZEFC_SEND* (object receivers):
//
//   Meson: -Dmethod_dispatch=ic|vtable  →  -DZEFC_METHOD_IC=1|0
//
//   ic (default): per-site CallSite — isa_ guard + cached callee (Zef-style).
//   vtable:       every send does isa_->slots[sel](…) (C++-virtual shape).
//
// Immediate Double/Int32 short-circuit in both modes. Field IC is separate
// (ZEFC_IC_GET/SET). Varying-selector send() is always pure vtable.

#ifndef ZEFC_METHOD_IC
#define ZEFC_METHOD_IC 1
#endif

#if ZEFC_METHOD_IC
#include "zefc/call_ic.hpp"
#endif

namespace zefc {

#define ZEFC_CONCAT2(a, b) a##b
#define ZEFC_CONCAT(a, b) ZEFC_CONCAT2(a, b)

#if ZEFC_METHOD_IC

// --- method IC: unique CallSite per expansion (statement-expr + __COUNTER__) ---
// Do not use template<__COUNTER__> — ODR-merges across TUs and crosses selectors.

#define ZEFC_SEND0(obj, sel) ZEFC_SEND0_I((obj), (sel), __COUNTER__)
#define ZEFC_SEND0_I(obj, sel, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call0((obj), &ZEFC_CONCAT(_zefc_cs_, N)); \
  })

#define ZEFC_SEND1(obj, sel, a1) ZEFC_SEND1_I((obj), (sel), (a1), __COUNTER__)
#define ZEFC_SEND1_I(obj, sel, a1, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call1((obj), &ZEFC_CONCAT(_zefc_cs_, N), (a1)); \
  })

#define ZEFC_SEND2(obj, sel, a1, a2) ZEFC_SEND2_I((obj), (sel), (a1), (a2), __COUNTER__)
#define ZEFC_SEND2_I(obj, sel, a1, a2, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call2((obj), &ZEFC_CONCAT(_zefc_cs_, N), (a1), (a2)); \
  })

#define ZEFC_SEND3(obj, sel, a1, a2, a3) \
  ZEFC_SEND3_I((obj), (sel), (a1), (a2), (a3), __COUNTER__)
#define ZEFC_SEND3_I(obj, sel, a1, a2, a3, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call3((obj), &ZEFC_CONCAT(_zefc_cs_, N), (a1), (a2), (a3)); \
  })

#define ZEFC_SEND4(obj, sel, a1, a2, a3, a4) \
  ZEFC_SEND4_I((obj), (sel), (a1), (a2), (a3), (a4), __COUNTER__)
#define ZEFC_SEND4_I(obj, sel, a1, a2, a3, a4, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call4((obj), &ZEFC_CONCAT(_zefc_cs_, N), (a1), (a2), (a3), (a4)); \
  })

#else // !ZEFC_METHOD_IC — pure vtable

#define ZEFC_SEND0(obj, sel) \
  (zefc::id_is_double(obj) \
     ? zefc::zefc_double_send0((obj), (sel)) \
     : (zefc::id_is_int32(obj) \
          ? zefc::zefc_int_send0((obj), (sel)) \
          : ((obj)->isa_->slots[(sel)]((obj), (sel)))))

#define ZEFC_SEND1(obj, sel, a1) \
  (zefc::id_is_double(obj) \
     ? zefc::zefc_double_send1((obj), (sel), (a1)) \
     : (zefc::id_is_int32(obj) \
          ? zefc::zefc_int_send1((obj), (sel), (a1)) \
          : ((obj)->isa_->slots[(sel)]((obj), (sel), (a1)))))

#define ZEFC_SEND2(obj, sel, a1, a2) \
  ((obj)->isa_->slots[(sel)]((obj), (sel), (a1), (a2)))

#define ZEFC_SEND3(obj, sel, a1, a2, a3) \
  ((obj)->isa_->slots[(sel)]((obj), (sel), (a1), (a2), (a3)))

#define ZEFC_SEND4(obj, sel, a1, a2, a3, a4) \
  ((obj)->isa_->slots[(sel)]((obj), (sel), (a1), (a2), (a3), (a4)))

#endif // ZEFC_METHOD_IC

// Varying selector (not a fixed call site) — always pure vtable. Prefer ZEFC_SEND*.
inline id send(id recv, int sel, id arg0)
{
  if (id_is_double(recv)) {
    return zefc_double_send1(recv, sel, arg0);
  }
  if (id_is_int32(recv)) {
    return zefc_int_send1(recv, sel, arg0);
  }
  return recv->isa_->slots[sel](recv, sel, arg0);
}

#define ZEFC_SITE(mangled_lit) \
  ([]() -> int { \
    static int cell = 0; \
    if (cell == 0) { \
      cell = zefc::selector_intern(mangled_lit); \
    } \
    return cell; \
  }())

} // namespace zefc
