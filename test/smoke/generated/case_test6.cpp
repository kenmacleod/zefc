// Generated from ../zef/tests/test6.zef (hand-maintained).
// Structure: Foo/Bar inheritance, field getters, doShit + super.doShit, toString.

#include <cstdarg>

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Foo_ {
  zefc_method* isa_;
  id thingy;
  id stuff;
};

struct Bar_ {
  zefc_method* isa_;
  id thingy;
  id stuff;
  id whatever;
  id blah;
};

static zefc_method Foo_vtable[kMaxSelectors];
static zefc_method Bar_vtable[kMaxSelectors];

static int slot_doShit = 0;
static int slot_thingy = 0;
static int slot_stuff = 0;
static int slot_whatever = 0;
static int slot_blah = 0;

static void
ensure_slots()
{
  if (slot_doShit != 0) {
    return;
  }
  selector_patch(&slot_doShit, selector_intern("doShit_o"));
  selector_patch(&slot_thingy, selector_intern("thingy_o"));
  selector_patch(&slot_stuff, selector_intern("stuff_o"));
  selector_patch(&slot_whatever, selector_intern("whatever_o"));
  selector_patch(&slot_blah, selector_intern("blah_o"));
}

static id
Foo__thingy_o(id self, int selector, ...)
{
  (void)selector;
  return body<Foo_>(self)->thingy;
}

static id
Foo__stuff_o(id self, int selector, ...)
{
  (void)selector;
  return body<Foo_>(self)->stuff;
}

static id
Foo__doShit_o(id self, int selector, ...)
{
  (void)selector;
  Foo_* f = body<Foo_>(self);
  f->thingy = send(f->thingy, zefc_slot_add_o, Int__from_i64(1));
  f->stuff = send(f->stuff, zefc_slot_add_o, String__from_utf8("x"));
  return null_id();
}

static id
Foo__toString_o(id self, int selector, ...)
{
  (void)selector;
  Foo_* f = body<Foo_>(self);
  id s = String__from_utf8("Foo<");
  s = send(s, zefc_slot_add_o, ZEFC_SEND0(f->thingy, zefc_slot_toString_o));
  s = send(s, zefc_slot_add_o, String__from_utf8(","));
  s = send(s, zefc_slot_add_o, ZEFC_SEND0(f->stuff, zefc_slot_toString_o));
  s = send(s, zefc_slot_add_o, String__from_utf8(">"));
  return s;
}

static id
Bar__whatever_o(id self, int selector, ...)
{
  (void)selector;
  return body<Bar_>(self)->whatever;
}

static id
Bar__blah_o(id self, int selector, ...)
{
  (void)selector;
  return body<Bar_>(self)->blah;
}

static id
Bar__doShit_o(id self, int selector, ...)
{
  (void)selector;
  // super.doShit
  Foo__doShit_o(self, slot_doShit);
  Bar_* b = body<Bar_>(self);
  b->whatever = send(b->whatever, zefc_slot_add_o, String__from_utf8("y"));
  b->blah = send(b->blah, zefc_slot_add_o, Int__from_i64(2));
  return null_id();
}

static id
Bar__toString_o(id self, int selector, ...)
{
  (void)selector;
  Bar_* b = body<Bar_>(self);
  id s = String__from_utf8("Bar<");
  s = send(s, zefc_slot_add_o, ZEFC_SEND0(b->thingy, zefc_slot_toString_o));
  s = send(s, zefc_slot_add_o, String__from_utf8(","));
  s = send(s, zefc_slot_add_o, ZEFC_SEND0(b->stuff, zefc_slot_toString_o));
  s = send(s, zefc_slot_add_o, String__from_utf8(","));
  s = send(s, zefc_slot_add_o, ZEFC_SEND0(b->whatever, zefc_slot_toString_o));
  s = send(s, zefc_slot_add_o, String__from_utf8(","));
  s = send(s, zefc_slot_add_o, ZEFC_SEND0(b->blah, zefc_slot_toString_o));
  s = send(s, zefc_slot_add_o, String__from_utf8(">"));
  return s;
}

static id
Bar__new(id crap)
{
  ensure_slots();
  static bool ready = false;
  if (!ready) {
    for (int i = 0; i < kMaxSelectors; ++i) {
      Foo_vtable[i] = doesNotUnderstand;
      Bar_vtable[i] = doesNotUnderstand;
    }
    vtable_set(Foo_vtable, slot_thingy, Foo__thingy_o);
    vtable_set(Foo_vtable, slot_stuff, Foo__stuff_o);
    vtable_set(Foo_vtable, slot_doShit, Foo__doShit_o);
    vtable_set(Foo_vtable, zefc_slot_toString_o, Foo__toString_o);

    // Bar inherits Foo getters for thingy/stuff layout-compatible prefix.
    vtable_set(Bar_vtable, slot_thingy, Foo__thingy_o);
    vtable_set(Bar_vtable, slot_stuff, Foo__stuff_o);
    vtable_set(Bar_vtable, slot_whatever, Bar__whatever_o);
    vtable_set(Bar_vtable, slot_blah, Bar__blah_o);
    vtable_set(Bar_vtable, slot_doShit, Bar__doShit_o);
    vtable_set(Bar_vtable, zefc_slot_toString_o, Bar__toString_o);
    ready = true;
  }

  Bar_* b = alloc<Bar_>();
  b->isa_ = Bar_vtable;
  // Bar(crap): whatever = crap.toString; blah = crap + 5; super(crap + 10)
  b->whatever = ZEFC_SEND0(crap, zefc_slot_toString_o);
  b->blah = send(crap, zefc_slot_add_o, Int__from_i64(5));
  id super_arg = send(crap, zefc_slot_add_o, Int__from_i64(10));
  b->thingy = super_arg;
  b->stuff = String__from_utf8("hello");
  return as_id(b);
}

} // namespace

void
smoke_test6()
{
  id bar = Bar__new(Int__from_i64(666));
  (void)ZEFC_SEND0(bar, slot_doShit);
  (void)ZEFC_SEND0(bar, slot_doShit);
  println(bar);
}

} // namespace smoke
} // namespace zefc
