// Generated from ../zef/tests/staticcall3.zef (hand-maintained).
// Structure: Foo().Bar() — instance nested class get then call.

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Foo_ {
  IsaPtr isa_;
  id Bar;
};

struct BarClass_ {
  IsaPtr isa_;
};

static VTable* Foo_vtable = nullptr;
static VTable* Bar_vtable = nullptr;
static BarClass_ g_Bar;
static int slot_Bar = 0;
static int slot_call = 0;

static id
Bar__call_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  return Int__from_i64(42);
}

static id
Foo__Bar_o(id self, int selector, ...)
{
  (void)selector;
  return body<Foo_>(self)->Bar;
}

static id
Foo__new()
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_Bar, selector_intern("sc3_Bar_o"));
    selector_patch(&slot_call, selector_intern("sc3_call_o"));
    if (!Foo_vtable) {
      Foo_vtable = vtable_create();
    }
    if (!Bar_vtable) {
      Bar_vtable = vtable_create();
    }
    vtable_set(Foo_vtable, slot_Bar, Foo__Bar_o);
    vtable_set(Bar_vtable, slot_call, Bar__call_o);
    zefc_set_isa(&g_Bar, Bar_vtable);
    ready = true;
  }
  Foo_* o = alloc<Foo_>();
  zefc_set_isa(o, Foo_vtable);
  o->Bar = as_id(&g_Bar);
  return as_id(o);
}

} // namespace

void
smoke_staticcall3()
{
  id Bar = ZEFC_SEND0(Foo__new(), slot_Bar);
  println(ZEFC_SEND0(Bar, slot_call));
}

} // namespace smoke
} // namespace zefc
