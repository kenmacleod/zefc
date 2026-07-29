// Generated from ../zef/tests/test9.zef (hand-maintained).
// Structure: Foo.stuff(z) → closure(w,a,b) → Bar.thingy summing captured ints.

#include <cstdarg>

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Foo_ {
  zefc_method* isa_;
  id x;
  id y;
};

struct InnerClosure_ {
  zefc_method* isa_;
  id x;
  id y;
  id z;
};

struct Bar_ {
  zefc_method* isa_;
  id x;
  id y;
  id z;
  id w;
  id a;
  id b;
};

static zefc_method Foo_vtable[kMaxSelectors];
static zefc_method InnerClosure_vtable[kMaxSelectors];
static zefc_method Bar_vtable[kMaxSelectors];

static int slot_stuff = 0;
static int slot_call3 = 0;
static int slot_thingy = 0;

static id
Bar__thingy_o(id self, int selector, ...)
{
  (void)selector;
  Bar_* b = body<Bar_>(self);
  id s = send(b->x, zefc_slot_add_o, b->y);
  s = send(s, zefc_slot_add_o, b->z);
  s = send(s, zefc_slot_add_o, b->w);
  s = send(s, zefc_slot_add_o, b->a);
  s = send(s, zefc_slot_add_o, b->b);
  return s;
}

static id
Bar__new(id x, id y, id z, id w, id a, id b)
{
  static bool ready = false;
  if (!ready) {
    if (slot_thingy == 0) {
      selector_patch(&slot_thingy, selector_intern("thingy_o"));
    }
    for (int i = 0; i < kMaxSelectors; ++i) {
      Bar_vtable[i] = doesNotUnderstand;
    }
    vtable_set(Bar_vtable, slot_thingy, Bar__thingy_o);
    ready = true;
  }
  Bar_* o = alloc<Bar_>();
  o->isa_ = Bar_vtable;
  o->x = x;
  o->y = y;
  o->z = z;
  o->w = w;
  o->a = a;
  o->b = b;
  return as_id(o);
}

static id
InnerClosure__call_ooo(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id w = va_arg(ap, id);
  id a = va_arg(ap, id);
  id b = va_arg(ap, id);
  va_end(ap);
  InnerClosure_* c = body<InnerClosure_>(self);
  return Bar__new(c->x, c->y, c->z, w, a, b);
}

static id
make_inner(id x, id y, id z)
{
  static bool ready = false;
  if (!ready) {
    if (slot_call3 == 0) {
      selector_patch(&slot_call3, selector_intern("call_ooo"));
    }
    for (int i = 0; i < kMaxSelectors; ++i) {
      InnerClosure_vtable[i] = doesNotUnderstand;
    }
    vtable_set(InnerClosure_vtable, slot_call3, InnerClosure__call_ooo);
    ready = true;
  }
  InnerClosure_* c = alloc<InnerClosure_>();
  c->isa_ = InnerClosure_vtable;
  c->x = x;
  c->y = y;
  c->z = z;
  return as_id(c);
}

static id
Foo__stuff_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id z = va_arg(ap, id);
  va_end(ap);
  Foo_* f = body<Foo_>(self);
  return make_inner(f->x, f->y, z);
}

static id
Foo__new(id inX, id inY)
{
  static bool ready = false;
  if (!ready) {
    if (slot_stuff == 0) {
      selector_patch(&slot_stuff, selector_intern("stuff_o"));
    }
    for (int i = 0; i < kMaxSelectors; ++i) {
      Foo_vtable[i] = doesNotUnderstand;
    }
    vtable_set(Foo_vtable, slot_stuff, Foo__stuff_o);
    ready = true;
  }
  Foo_* f = alloc<Foo_>();
  f->isa_ = Foo_vtable;
  f->x = inX;
  f->y = inY;
  return as_id(f);
}

} // namespace

void
smoke_test9()
{
  id foo = Foo__new(Int__from_i64(1), Int__from_i64(2));
  id inner = send(foo, slot_stuff, Int__from_i64(3));
  id bar = body<InnerClosure_>(inner)->isa_[slot_call3](
      inner, slot_call3, Int__from_i64(4), Int__from_i64(5), Int__from_i64(6));
  println(ZEFC_SEND0(bar, slot_thingy));
}

} // namespace smoke
} // namespace zefc
