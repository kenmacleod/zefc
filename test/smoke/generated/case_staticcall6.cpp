// Generated from ../zef/tests/staticcall6.zef (hand-maintained).
// Structure: Foo's static call is a nested callable class; Foo() → that.call().

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Class_ {
  IsaPtr isa_;
  id call_nested;
};

static VTable* Foo_vtable = nullptr;
static VTable* Nested_vtable = nullptr;
static Class_ g_Foo;
static Class_ g_Nested;
static int slot_call = 0;

static id
Nested__call_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  return Int__from_i64(42);
}

static id
Foo__call_o(id self, int selector, ...)
{
  (void)selector;
  // Foo() → invoke nested call class
  id nested = body<Class_>(self)->call_nested;
  return ZEFC_SEND0(nested, slot_call);
}

static void
ensure()
{
  if (slot_call != 0) {
    return;
  }
  selector_patch(&slot_call, selector_intern("sc6_call_o"));
  if (!Foo_vtable) {
      Foo_vtable = vtable_create();
    }
    if (!Nested_vtable) {
      Nested_vtable = vtable_create();
    }
  vtable_set(Nested_vtable, slot_call, Nested__call_o);
  vtable_set(Foo_vtable, slot_call, Foo__call_o);
  zefc_set_isa(&g_Nested, Nested_vtable);
  g_Nested.call_nested = null_id();
  zefc_set_isa(&g_Foo, Foo_vtable);
  g_Foo.call_nested = as_id(&g_Nested);
}

} // namespace

void
smoke_staticcall6()
{
  ensure();
  println(ZEFC_SEND0(as_id(&g_Foo), slot_call));
}

} // namespace smoke
} // namespace zefc
