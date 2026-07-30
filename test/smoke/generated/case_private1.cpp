// Generated from ../zef/tests/private1.zef (hand-maintained).
// Structure: public thingy sends to private stuff.

#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Foo_ {
  IsaPtr isa_;
};

static VTable* Foo_vtable = nullptr;
static int slot_thingy = 0;
static int slot_stuff = 0;

static id
Foo__stuff_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  println(String__from_utf8("Hello"));
  return null_id();
}

static id
Foo__thingy_o(id self, int selector, ...)
{
  (void)selector;
  return ZEFC_SEND0(self, slot_stuff);
}

static id
Foo__new()
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_thingy, selector_intern("priv1_thingy_o"));
    selector_patch(&slot_stuff, selector_intern("priv1_stuff_o"));
    if (!Foo_vtable) {
      Foo_vtable = vtable_create();
    }
    vtable_set(Foo_vtable, slot_thingy, Foo__thingy_o);
    vtable_set(Foo_vtable, slot_stuff, Foo__stuff_o);
    ready = true;
  }
  Foo_* o = alloc<Foo_>();
  zefc_set_isa(o, Foo_vtable);
  return as_id(o);
}

} // namespace

void
smoke_private1()
{
  (void)ZEFC_SEND0(Foo__new(), slot_thingy);
}

} // namespace smoke
} // namespace zefc
