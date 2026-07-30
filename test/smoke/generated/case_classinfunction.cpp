// Generated from ../zef/tests/classinfunction.zef (hand-maintained).
// Structure: fn returns class; ctor captures outer arg; readable baz.

#include <cstdarg>

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct WTF_ {
  IsaPtr isa_;
  id baz;
};

struct WTFClass_ {
  IsaPtr isa_;
  id captured_arg;
};

static VTable* WTF_vtable = nullptr;
static VTable* WTFClass_vtable = nullptr;
static int slot_baz = 0;
static int slot_call = 0; // construct: Class(inBaz)

static id
WTF__baz_o(id self, int selector, ...)
{
  (void)selector;
  return body<WTF_>(self)->baz;
}

static id
WTFClass__call_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id inBaz = va_arg(ap, id);
  va_end(ap);
  WTFClass_* cls = body<WTFClass_>(self);
  static bool inst_ready = false;
  if (!inst_ready) {
    if (slot_baz == 0) {
      selector_patch(&slot_baz, selector_intern("baz_o"));
    }
    if (!WTF_vtable) {
      WTF_vtable = vtable_create();
    }
    vtable_set(WTF_vtable, slot_baz, WTF__baz_o);
    inst_ready = true;
  }
  WTF_* o = alloc<WTF_>();
  zefc_set_isa(o, WTF_vtable);
  o->baz = send(inBaz, ZEFC_SITE("add_o"), cls->captured_arg);
  return as_id(o);
}

static id
thingy(id arg)
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_call, selector_intern("wtf_call_o"));
    if (!WTFClass_vtable) {
      WTFClass_vtable = vtable_create();
    }
    vtable_set(WTFClass_vtable, slot_call, WTFClass__call_o);
    ready = true;
  }
  WTFClass_* cls = alloc<WTFClass_>();
  zefc_set_isa(cls, WTFClass_vtable);
  cls->captured_arg = arg;
  return as_id(cls);
}

} // namespace

void
smoke_classinfunction()
{
  id cls = thingy(Int__from_i64(42));
  id inst = send(cls, slot_call, Int__from_i64(666));
  println(ZEFC_SEND0(inst, slot_baz));
}

} // namespace smoke
} // namespace zefc
