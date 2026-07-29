#pragma once

#include "zefc/runtime.hpp"

namespace zefc {

// Hot path: vptr (VTable*) → slots[sel] → call.
// sel is a per-site patched cell (ZEFC_SITE). Ideal end state: instruction-immediate.

#define ZEFC_SEND0(obj, sel) \
  ((obj)->isa_->slots[(sel)]((obj), (sel)))

#define ZEFC_SEND1(obj, sel, a1) \
  ((obj)->isa_->slots[(sel)]((obj), (sel), (a1)))

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
