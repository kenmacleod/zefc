// Generated from ../zef/tests/staticcall.zef (hand-maintained).
// Structure: class Foo with static call; Foo() → send call.

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct FooClass_ {
  VTable* isa_;
};

static VTable* FooClass_vtable = nullptr;
static FooClass_ g_Foo;
static int slot_call = 0;

static id
Foo__call_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  return Int__from_i64(42);
}

static void
ensure()
{
  if (slot_call != 0) {
    return;
  }
  selector_patch(&slot_call, selector_intern("sc_call_o"));
  if (!FooClass_vtable) {
      FooClass_vtable = vtable_create();
    }
  vtable_set(FooClass_vtable, slot_call, Foo__call_o);
  g_Foo.isa_ = FooClass_vtable;
}

} // namespace

void
smoke_staticcall()
{
  ensure();
  println(ZEFC_SEND0(as_id(&g_Foo), slot_call));
}

} // namespace smoke
} // namespace zefc
