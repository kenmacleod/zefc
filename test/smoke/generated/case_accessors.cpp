// Generated from ../zef/tests/accessors.zef (hand-maintained).
// Structure: readable/accessible instance + static fields via get/set sends.

#include <cstdarg>

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Foo_ {
  zefc_method* isa_;
  id a;
  id b;
};

struct FooClass_ {
  zefc_method* isa_;
  id c;
  id d;
};

static zefc_method Foo_vtable[kMaxSelectors];
static zefc_method FooClass_vtable[kMaxSelectors];
static FooClass_ g_FooClass;
static int slot_a = 0;
static int slot_b = 0;
static int slot_set_b = 0;
static int slot_c = 0;
static int slot_d = 0;
static int slot_set_d = 0;

static id Foo__a_o(id self, int, ...) { return body<Foo_>(self)->a; }
static id Foo__b_o(id self, int, ...) { return body<Foo_>(self)->b; }

static id
Foo__set_b_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  body<Foo_>(self)->b = v;
  return null_id();
}

static id FooClass__c_o(id self, int, ...) { return body<FooClass_>(self)->c; }
static id FooClass__d_o(id self, int, ...) { return body<FooClass_>(self)->d; }

static id
FooClass__set_d_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  body<FooClass_>(self)->d = v;
  return null_id();
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
  for (int i = 0; i < kMaxSelectors; ++i) {
    Foo_vtable[i] = doesNotUnderstand;
    FooClass_vtable[i] = doesNotUnderstand;
  }
  vtable_set(Foo_vtable, slot_a, Foo__a_o);
  vtable_set(Foo_vtable, slot_b, Foo__b_o);
  vtable_set(Foo_vtable, slot_set_b, Foo__set_b_o);
  vtable_set(FooClass_vtable, slot_c, FooClass__c_o);
  vtable_set(FooClass_vtable, slot_d, FooClass__d_o);
  vtable_set(FooClass_vtable, slot_set_d, FooClass__set_d_o);
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
  println(ZEFC_SEND0(o, slot_a));
  println(ZEFC_SEND0(o, slot_b));
  (void)ZEFC_SEND1(o, slot_set_b, Int__from_i64(42));
  println(ZEFC_SEND0(o, slot_b));

  id Foo = as_id(&g_FooClass);
  println(ZEFC_SEND0(Foo, slot_c));
  println(ZEFC_SEND0(Foo, slot_d));
  (void)ZEFC_SEND1(Foo, slot_set_d, Int__from_i64(666));
  println(ZEFC_SEND0(Foo, slot_d));
}

} // namespace smoke
} // namespace zefc
