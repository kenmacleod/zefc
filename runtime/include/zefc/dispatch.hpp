#pragma once

#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"

namespace zefc {

// Hot path: vptr (VTable*) → slots[sel] → call.
// sel should be a patched site cell (ZEFC_SITE) or, during migration, a
// once-patched compat global. Ideal end state: instruction-immediate sel.

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

// Per-call-site patch cell. Registers with the pending patch set; call
// selector_sites_patch() (via module_load / runtime init) before first send.
#define ZEFC_SITE(mangled_lit) \
  ([]() -> int& { \
    static int cell = 0; \
    static bool registered = false; \
    if (!registered) { \
      zefc::selector_site_register(&cell, mangled_lit); \
      registered = true; \
    } \
    return cell; \
  }())

} // namespace zefc
