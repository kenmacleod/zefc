// Generated from ../zef/tests/private2.zef (hand-maintained).
// Structure: static thingy → static private stuff on class object.

#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct FooClass_ {
  zefc_method* isa_;
};

static zefc_method FooClass_vtable[kMaxSelectors];
static FooClass_ g_Foo;
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

static void
ensure()
{
  if (slot_thingy != 0) {
    return;
  }
  selector_patch(&slot_thingy, selector_intern("priv2_thingy_o"));
  selector_patch(&slot_stuff, selector_intern("priv2_stuff_o"));
  for (int i = 0; i < kMaxSelectors; ++i) {
    FooClass_vtable[i] = doesNotUnderstand;
  }
  vtable_set(FooClass_vtable, slot_thingy, Foo__thingy_o);
  vtable_set(FooClass_vtable, slot_stuff, Foo__stuff_o);
  g_Foo.isa_ = FooClass_vtable;
}

} // namespace

void
smoke_private2()
{
  ensure();
  (void)ZEFC_SEND0(as_id(&g_Foo), slot_thingy);
}

} // namespace smoke
} // namespace zefc
