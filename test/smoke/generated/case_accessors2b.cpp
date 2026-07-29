// Generated from ../zef/tests/accessors2b.zef (hand-maintained).
// Structure: private a/b accessed only via same-class methods thingy/stuff/whatever.

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

static zefc_method Foo_vtable[kMaxSelectors];
static int slot_a = 0;
static int slot_b = 0;
static int slot_set_b = 0;
static int slot_thingy = 0;
static int slot_stuff = 0;
static int slot_whatever = 0;

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

static id
Foo__thingy_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id o = va_arg(ap, id);
  va_end(ap);
  (void)self;
  return ZEFC_SEND0(o, slot_a);
}

static id
Foo__stuff_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id o = va_arg(ap, id);
  va_end(ap);
  (void)self;
  return ZEFC_SEND0(o, slot_b);
}

static id
Foo__whatever_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id o = va_arg(ap, id);
  id v = va_arg(ap, id);
  va_end(ap);
  (void)self;
  return ZEFC_SEND1(o, slot_set_b, v);
}

static id
Foo__new()
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_a, selector_intern("priv_a_o"));
    selector_patch(&slot_b, selector_intern("priv_b_o"));
    selector_patch(&slot_set_b, selector_intern("priv_set_b_o"));
    selector_patch(&slot_thingy, selector_intern("thingy_o1"));
    selector_patch(&slot_stuff, selector_intern("stuff_o1"));
    selector_patch(&slot_whatever, selector_intern("whatever_oo"));
    for (int i = 0; i < kMaxSelectors; ++i) {
      Foo_vtable[i] = doesNotUnderstand;
    }
    vtable_set(Foo_vtable, slot_a, Foo__a_o);
    vtable_set(Foo_vtable, slot_b, Foo__b_o);
    vtable_set(Foo_vtable, slot_set_b, Foo__set_b_o);
    vtable_set(Foo_vtable, slot_thingy, Foo__thingy_o);
    vtable_set(Foo_vtable, slot_stuff, Foo__stuff_o);
    vtable_set(Foo_vtable, slot_whatever, Foo__whatever_o);
    ready = true;
  }
  Foo_* o = alloc<Foo_>();
  o->isa_ = Foo_vtable;
  o->a = Int__from_i64(1);
  o->b = Int__from_i64(2);
  return as_id(o);
}

} // namespace

void
smoke_accessors2b()
{
  id o = Foo__new();
  println(send(Foo__new(), slot_thingy, o));
  println(send(Foo__new(), slot_stuff, o));
  (void)body<Foo_>(Foo__new())->isa_[slot_whatever](
      Foo__new(), slot_whatever, o, Int__from_i64(42));
  println(send(Foo__new(), slot_stuff, o));
}

} // namespace smoke
} // namespace zefc
