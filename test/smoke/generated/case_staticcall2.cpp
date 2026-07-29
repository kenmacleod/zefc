// Generated from ../zef/tests/staticcall2.zef (hand-maintained).
// Structure: Foo.Bar nested static class; Foo.Bar() → call.

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Class_ {
  zefc_method* isa_;
  id nested_Bar;
};

static zefc_method Foo_vtable[kMaxSelectors];
static zefc_method Bar_vtable[kMaxSelectors];
static Class_ g_Foo;
static Class_ g_Bar;
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
  return body<Class_>(self)->nested_Bar;
}

static void
ensure()
{
  if (slot_call != 0) {
    return;
  }
  selector_patch(&slot_Bar, selector_intern("sc2_Bar_o"));
  selector_patch(&slot_call, selector_intern("sc2_call_o"));
  for (int i = 0; i < kMaxSelectors; ++i) {
    Foo_vtable[i] = doesNotUnderstand;
    Bar_vtable[i] = doesNotUnderstand;
  }
  vtable_set(Bar_vtable, slot_call, Bar__call_o);
  vtable_set(Foo_vtable, slot_Bar, Foo__Bar_o);
  g_Bar.isa_ = Bar_vtable;
  g_Bar.nested_Bar = null_id();
  g_Foo.isa_ = Foo_vtable;
  g_Foo.nested_Bar = as_id(&g_Bar);
}

} // namespace

void
smoke_staticcall2()
{
  ensure();
  id Bar = ZEFC_SEND0(as_id(&g_Foo), slot_Bar);
  println(ZEFC_SEND0(Bar, slot_call));
}

} // namespace smoke
} // namespace zefc
