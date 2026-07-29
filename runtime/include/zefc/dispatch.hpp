#pragma once

#include "zefc/double_api.hpp"
#include "zefc/int_api.hpp"
#include "zefc/runtime.hpp"

namespace zefc {

// Hot path: vptr (VTable*) → slots[sel] → call.
// Immediate Double/Int32 short-circuit (Zef Value ops) — no isa_ load.
// Known names: ZEFC_SEL_* literals. Dynamic names: ZEFC_SITE cells.

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

inline id send(id recv, int sel, id arg0)
{
  return ZEFC_SEND1(recv, sel, arg0);
}

// Per-call-site selector cell. First use interns (and grows vtables); later
// uses only load this site's static. Path toward instruction-immediate patching.
#define ZEFC_SITE(mangled_lit) \
  ([]() -> int { \
    static int cell = 0; \
    if (cell == 0) { \
      cell = zefc::selector_intern(mangled_lit); \
    } \
    return cell; \
  }())

} // namespace zefc
