// Generated from ../zef/tests/private3.zef (hand-maintained).
// Structure: Foo.create → instance; .thingy send. Private ctor modeled as factory-only.

#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Foo_ {
  zefc_method* isa_;
};

struct FooClass_ {
  zefc_method* isa_;
};

static zefc_method Foo_vtable[kMaxSelectors];
static zefc_method FooClass_vtable[kMaxSelectors];
static FooClass_ g_Foo;
static int slot_create = 0;
static int slot_thingy = 0;

static id
Foo__thingy_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  println(String__from_utf8("Hello"));
  return null_id();
}

static id
Foo__new()
{
  Foo_* o = alloc<Foo_>();
  o->isa_ = Foo_vtable;
  return as_id(o);
}

static id
FooClass__create_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  return Foo__new();
}

static void
ensure()
{
  if (slot_create != 0) {
    return;
  }
  selector_patch(&slot_create, selector_intern("create_o"));
  selector_patch(&slot_thingy, selector_intern("priv3_thingy_o"));
  for (int i = 0; i < kMaxSelectors; ++i) {
    Foo_vtable[i] = doesNotUnderstand;
    FooClass_vtable[i] = doesNotUnderstand;
  }
  vtable_set(Foo_vtable, slot_thingy, Foo__thingy_o);
  vtable_set(FooClass_vtable, slot_create, FooClass__create_o);
  g_Foo.isa_ = FooClass_vtable;
}

} // namespace

void
smoke_private3()
{
  ensure();
  id inst = ZEFC_SEND0(as_id(&g_Foo), slot_create);
  (void)ZEFC_SEND0(inst, slot_thingy);
}

} // namespace smoke
} // namespace zefc
