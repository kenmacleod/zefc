#pragma once

#include "zefc/double_api.hpp"
#include "zefc/field_ic.hpp"
#include "zefc/int_api.hpp"
#include "zefc/object_dispatch.hpp"
#include "zefc/runtime.hpp"

// Object ZEFC_SEND* — one API, four compile-time shapes (see object_dispatch.hpp):
//
//   Meson -Dobject_dispatch=slots|ic|flat|site  →  -DZEFC_OBJECT_DISPATCH=N
//
// Immediate Double/Int32 short-circuit in all modes. Field IC unchanged.
// Varying-selector send() always uses zefc_method_at (table path).

#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_IC || ZEFC_OBJECT_DISPATCH == ZEFC_OD_SITE
#include "zefc/call_ic.hpp"
#endif

namespace zefc {

#define ZEFC_CONCAT2(a, b) a##b
#define ZEFC_CONCAT(a, b) ZEFC_CONCAT2(a, b)

#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_IC

// --- guarded method IC ---
// Evaluate obj before sel so ctors that intern selectors (Foo__new) run first.
#define ZEFC_SEND0(obj, sel) ZEFC_SEND0_I((obj), (sel), __COUNTER__)
#define ZEFC_SEND0_I(obj, sel, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call0(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N)); \
  })

#define ZEFC_SEND1(obj, sel, a1) ZEFC_SEND1_I((obj), (sel), (a1), __COUNTER__)
#define ZEFC_SEND1_I(obj, sel, a1, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::id ZEFC_CONCAT(_zefc_a1_, N) = (a1); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call1(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N), \
                     ZEFC_CONCAT(_zefc_a1_, N)); \
  })

#define ZEFC_SEND2(obj, sel, a1, a2) ZEFC_SEND2_I((obj), (sel), (a1), (a2), __COUNTER__)
#define ZEFC_SEND2_I(obj, sel, a1, a2, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::id ZEFC_CONCAT(_zefc_a1_, N) = (a1); \
    zefc::id ZEFC_CONCAT(_zefc_a2_, N) = (a2); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call2(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N), \
                     ZEFC_CONCAT(_zefc_a1_, N), ZEFC_CONCAT(_zefc_a2_, N)); \
  })

#define ZEFC_SEND3(obj, sel, a1, a2, a3) \
  ZEFC_SEND3_I((obj), (sel), (a1), (a2), (a3), __COUNTER__)
#define ZEFC_SEND3_I(obj, sel, a1, a2, a3, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::id ZEFC_CONCAT(_zefc_a1_, N) = (a1); \
    zefc::id ZEFC_CONCAT(_zefc_a2_, N) = (a2); \
    zefc::id ZEFC_CONCAT(_zefc_a3_, N) = (a3); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call3(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N), \
                     ZEFC_CONCAT(_zefc_a1_, N), ZEFC_CONCAT(_zefc_a2_, N), \
                     ZEFC_CONCAT(_zefc_a3_, N)); \
  })

#define ZEFC_SEND4(obj, sel, a1, a2, a3, a4) \
  ZEFC_SEND4_I((obj), (sel), (a1), (a2), (a3), (a4), __COUNTER__)
#define ZEFC_SEND4_I(obj, sel, a1, a2, a3, a4, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::id ZEFC_CONCAT(_zefc_a1_, N) = (a1); \
    zefc::id ZEFC_CONCAT(_zefc_a2_, N) = (a2); \
    zefc::id ZEFC_CONCAT(_zefc_a3_, N) = (a3); \
    zefc::id ZEFC_CONCAT(_zefc_a4_, N) = (a4); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_call4(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N), \
                     ZEFC_CONCAT(_zefc_a1_, N), ZEFC_CONCAT(_zefc_a2_, N), \
                     ZEFC_CONCAT(_zefc_a3_, N), ZEFC_CONCAT(_zefc_a4_, N)); \
  })

#elif ZEFC_OBJECT_DISPATCH == ZEFC_OD_SITE

// --- plan B: sticky site callee (fill on first use; no guard) ---
#define ZEFC_SEND0(obj, sel) ZEFC_SEND0_I((obj), (sel), __COUNTER__)
#define ZEFC_SEND0_I(obj, sel, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_site_call0(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N)); \
  })

#define ZEFC_SEND1(obj, sel, a1) ZEFC_SEND1_I((obj), (sel), (a1), __COUNTER__)
#define ZEFC_SEND1_I(obj, sel, a1, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::id ZEFC_CONCAT(_zefc_a1_, N) = (a1); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_site_call1(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N), \
                          ZEFC_CONCAT(_zefc_a1_, N)); \
  })

#define ZEFC_SEND2(obj, sel, a1, a2) ZEFC_SEND2_I((obj), (sel), (a1), (a2), __COUNTER__)
#define ZEFC_SEND2_I(obj, sel, a1, a2, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::id ZEFC_CONCAT(_zefc_a1_, N) = (a1); \
    zefc::id ZEFC_CONCAT(_zefc_a2_, N) = (a2); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_site_call2(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N), \
                          ZEFC_CONCAT(_zefc_a1_, N), ZEFC_CONCAT(_zefc_a2_, N)); \
  })

#define ZEFC_SEND3(obj, sel, a1, a2, a3) \
  ZEFC_SEND3_I((obj), (sel), (a1), (a2), (a3), __COUNTER__)
#define ZEFC_SEND3_I(obj, sel, a1, a2, a3, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::id ZEFC_CONCAT(_zefc_a1_, N) = (a1); \
    zefc::id ZEFC_CONCAT(_zefc_a2_, N) = (a2); \
    zefc::id ZEFC_CONCAT(_zefc_a3_, N) = (a3); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_site_call3(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N), \
                          ZEFC_CONCAT(_zefc_a1_, N), ZEFC_CONCAT(_zefc_a2_, N), \
                          ZEFC_CONCAT(_zefc_a3_, N)); \
  })

#define ZEFC_SEND4(obj, sel, a1, a2, a3, a4) \
  ZEFC_SEND4_I((obj), (sel), (a1), (a2), (a3), (a4), __COUNTER__)
#define ZEFC_SEND4_I(obj, sel, a1, a2, a3, a4, N) \
  ({ \
    static zefc::CallSite ZEFC_CONCAT(_zefc_cs_, N){0, nullptr, nullptr}; \
    zefc::id ZEFC_CONCAT(_zefc_r_, N) = (obj); \
    zefc::id ZEFC_CONCAT(_zefc_a1_, N) = (a1); \
    zefc::id ZEFC_CONCAT(_zefc_a2_, N) = (a2); \
    zefc::id ZEFC_CONCAT(_zefc_a3_, N) = (a3); \
    zefc::id ZEFC_CONCAT(_zefc_a4_, N) = (a4); \
    zefc::call_site_ensure_sel(&ZEFC_CONCAT(_zefc_cs_, N), (sel)); \
    zefc::zefc_site_call4(ZEFC_CONCAT(_zefc_r_, N), &ZEFC_CONCAT(_zefc_cs_, N), \
                          ZEFC_CONCAT(_zefc_a1_, N), ZEFC_CONCAT(_zefc_a2_, N), \
                          ZEFC_CONCAT(_zefc_a3_, N), ZEFC_CONCAT(_zefc_a4_, N)); \
  })

#elif ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT

// --- plan A: flat isa_[sel] ---
#define ZEFC_SEND0(obj, sel) \
  (zefc::id_is_double(obj) \
     ? zefc::zefc_double_send0((obj), (sel)) \
     : (zefc::id_is_int32(obj) \
          ? zefc::zefc_int_send0((obj), (sel)) \
          : ((obj)->isa_[(sel)]((obj), (sel)))))

#define ZEFC_SEND1(obj, sel, a1) \
  (zefc::id_is_double(obj) \
     ? zefc::zefc_double_send1((obj), (sel), (a1)) \
     : (zefc::id_is_int32(obj) \
          ? zefc::zefc_int_send1((obj), (sel), (a1)) \
          : ((obj)->isa_[(sel)]((obj), (sel), (a1)))))

#define ZEFC_SEND2(obj, sel, a1, a2) \
  ((obj)->isa_[(sel)]((obj), (sel), (a1), (a2)))

#define ZEFC_SEND3(obj, sel, a1, a2, a3) \
  ((obj)->isa_[(sel)]((obj), (sel), (a1), (a2), (a3)))

#define ZEFC_SEND4(obj, sel, a1, a2, a3, a4) \
  ((obj)->isa_[(sel)]((obj), (sel), (a1), (a2), (a3), (a4)))

#else // ZEFC_OD_SLOTS

// --- VTable* handle + slots[sel] ---
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

#endif // ZEFC_OBJECT_DISPATCH

inline id send(id recv, int sel, id arg0)
{
  if (id_is_double(recv)) {
    return zefc_double_send1(recv, sel, arg0);
  }
  if (id_is_int32(recv)) {
    return zefc_int_send1(recv, sel, arg0);
  }
  return zefc_method_at(recv, sel)(recv, sel, arg0);
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
