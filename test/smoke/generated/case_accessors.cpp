// Generated from ../zef/tests/accessors.zef (hand-maintained).
// Structure: readable/accessible instance + static fields via field IC.

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

#include <cstddef>

namespace zefc {
namespace smoke {

namespace {

struct Foo_ {
  VTable* isa_;
  id a;
  id b;
};

struct FooClass_ {
  VTable* isa_;
  id c;
  id d;
};

static VTable* Foo_vtable = nullptr;
static VTable* FooClass_vtable = nullptr;
static FooClass_ g_FooClass;
static int slot_a = 0;
static int slot_b = 0;
static int slot_set_b = 0;
static int slot_c = 0;
static int slot_d = 0;
static int slot_set_d = 0;

static id Foo_get_a(id self) { return body<Foo_>(self)->a; }
static id Foo_get_b(id self) { return body<Foo_>(self)->b; }
static id Foo_set_b(id self, id v) { body<Foo_>(self)->b = v; return null_id(); }
static id FooClass_get_c(id self) { return body<FooClass_>(self)->c; }
static id FooClass_get_d(id self) { return body<FooClass_>(self)->d; }
static id FooClass_set_d(id self, id v) { body<FooClass_>(self)->d = v; return null_id(); }

static id Foo__a_o(id self, int, ...) { return Foo_get_a(self); }
static id Foo__b_o(id self, int, ...) { return Foo_get_b(self); }
static id Foo__set_b_o(id self, int selector, id v)
{
  (void)selector;
  return Foo_set_b(self, v);
}
static id FooClass__c_o(id self, int, ...) { return FooClass_get_c(self); }
static id FooClass__d_o(id self, int, ...) { return FooClass_get_d(self); }
static id FooClass__set_d_o(id self, int selector, id v)
{
  (void)selector;
  return FooClass_set_d(self, v);
}

static void
ensure()
{
  if (slot_a != 0) {
    return;
  }
  selector_patch(&slot_a, selector_intern("a_o"));
  selector_patch(&slot_b, selector_intern("b_o"));
  selector_patch(&slot_set_b, selector_intern("set_b_o"));
  selector_patch(&slot_c, selector_intern("c_o"));
  selector_patch(&slot_d, selector_intern("d_o"));
  selector_patch(&slot_set_d, selector_intern("set_d_o"));
  if (!Foo_vtable) {
    Foo_vtable = vtable_create();
  }
  if (!FooClass_vtable) {
    FooClass_vtable = vtable_create();
  }
  vtable_set(Foo_vtable, slot_a, Foo__a_o);
  vtable_set(Foo_vtable, slot_b, Foo__b_o);
  vtable_set(Foo_vtable, slot_set_b, Foo__set_b_o);
  vtable_set(FooClass_vtable, slot_c, FooClass__c_o);
  vtable_set(FooClass_vtable, slot_d, FooClass__d_o);
  vtable_set(FooClass_vtable, slot_set_d, FooClass__set_d_o);
  field_register_get(Foo_vtable, slot_a, offsetof(Foo_, a));
  field_register_get(Foo_vtable, slot_b, offsetof(Foo_, b));
  field_register_set(Foo_vtable, slot_set_b, offsetof(Foo_, b));
  field_register_get(FooClass_vtable, slot_c, offsetof(FooClass_, c));
  field_register_get(FooClass_vtable, slot_d, offsetof(FooClass_, d));
  field_register_set(FooClass_vtable, slot_set_d, offsetof(FooClass_, d));
  g_FooClass.isa_ = FooClass_vtable;
  g_FooClass.c = Int__from_i64(3);
  g_FooClass.d = Int__from_i64(4);
}

static id
Foo__new()
{
  ensure();
  Foo_* o = alloc<Foo_>();
  o->isa_ = Foo_vtable;
  o->a = Int__from_i64(1);
  o->b = Int__from_i64(2);
  return as_id(o);
}

} // namespace

void
smoke_accessors()
{
  ensure();
  id o = Foo__new();
  println(ZEFC_IC_GET(o, slot_a, Foo_, a));
  println(ZEFC_IC_GET(o, slot_b, Foo_, b));
  (void)ZEFC_IC_SET(o, slot_set_b, Foo_, b, Int__from_i64(42));
  println(ZEFC_IC_GET(o, slot_b, Foo_, b));

  id Foo = as_id(&g_FooClass);
  println(ZEFC_IC_GET(Foo, slot_c, FooClass_, c));
  println(ZEFC_IC_GET(Foo, slot_d, FooClass_, d));
  (void)ZEFC_IC_SET(Foo, slot_set_d, FooClass_, d, Int__from_i64(666));
  println(ZEFC_IC_GET(Foo, slot_d, FooClass_, d));
}

} // namespace smoke
} // namespace zefc
