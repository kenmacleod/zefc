#pragma once

#include "zefc/call_ic.hpp"
#include "zefc/field_ic.hpp"
#include "zefc/runtime.hpp"

namespace zefc {

// Object sends: per-site method IC (guard isa_ + cached callee).
// Immediate Double/Int32 short-circuit inside zefc_call*.
//
// Unique CallSite per expansion: statement-expr + __COUNTER__ name suffix.
// Do not use template<__COUNTER__> for the site — that ODR-merges across TUs
// and crosses selectors (e.g. Array GET_i vs Double add_o).

#define ZEFC_CONCAT2(a, b) a##b
#define ZEFC_CONCAT(a, b) ZEFC_CONCAT2(a, b)

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

// Varying selector (not a fixed call site) — no IC. Prefer ZEFC_SEND* at fixed sites.
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
