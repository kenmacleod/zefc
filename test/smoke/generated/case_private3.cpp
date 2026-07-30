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
  IsaPtr isa_;
};

struct FooClass_ {
  IsaPtr isa_;
};

static VTable* Foo_vtable = nullptr;
static VTable* FooClass_vtable = nullptr;
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
  zefc_set_isa(o, Foo_vtable);
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
  if (!Foo_vtable) {
      Foo_vtable = vtable_create();
    }
    if (!FooClass_vtable) {
      FooClass_vtable = vtable_create();
    }
  vtable_set(Foo_vtable, slot_thingy, Foo__thingy_o);
  vtable_set(FooClass_vtable, slot_create, FooClass__create_o);
  zefc_set_isa(&g_Foo, FooClass_vtable);
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
