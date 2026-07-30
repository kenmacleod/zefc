// Generated from ../zef/tests/super.zef (hand-maintained).
// Structure: Foo/Bar instances, override foo, super.foo via superclass method.

#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Obj_ {
  IsaPtr isa_;
};

static VTable* Foo_vtable = nullptr;
static VTable* Bar_vtable = nullptr;
static int slot_foo = 0;
static int slot_bar = 0;

static id
Foo__foo_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  println(String__from_utf8("hello"));
  return null_id();
}

static id
Bar__foo_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  println(String__from_utf8("world"));
  return null_id();
}

static id
Bar__bar_o(id self, int selector, ...)
{
  (void)selector;
  // super.foo
  return Foo__foo_o(self, slot_foo);
}

static void
ensure_vtables()
{
  if (slot_foo != 0) {
    return;
  }
  selector_patch(&slot_foo, selector_intern("foo_o"));
  selector_patch(&slot_bar, selector_intern("bar_o"));
  if (!Foo_vtable) {
      Foo_vtable = vtable_create();
    }
    if (!Bar_vtable) {
      Bar_vtable = vtable_create();
    }
  vtable_set(Foo_vtable, slot_foo, Foo__foo_o);
  vtable_set(Bar_vtable, slot_foo, Bar__foo_o);
  vtable_set(Bar_vtable, slot_bar, Bar__bar_o);
}

static id
Foo__new()
{
  ensure_vtables();
  Obj_* o = alloc<Obj_>();
  zefc_set_isa(o, Foo_vtable);
  return as_id(o);
}

static id
Bar__new()
{
  ensure_vtables();
  Obj_* o = alloc<Obj_>();
  zefc_set_isa(o, Bar_vtable);
  return as_id(o);
}

static void
test()
{
  (void)ZEFC_SEND0(Foo__new(), slot_foo);
  (void)ZEFC_SEND0(Bar__new(), slot_bar);
  (void)ZEFC_SEND0(Bar__new(), slot_foo);
}

} // namespace

void
smoke_super()
{
  test();
  test();
}

} // namespace smoke
} // namespace zefc
