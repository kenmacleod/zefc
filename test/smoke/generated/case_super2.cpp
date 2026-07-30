// Generated from ../zef/tests/super2.zef (hand-maintained).
// Structure: Foo/Bar with foo(x); super.foo(x); override prints world.

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

struct Obj_ {
  IsaPtr isa_;
};

static VTable* Foo_vtable = nullptr;
static VTable* Bar_vtable = nullptr;
static int slot_foo = 0;
static int slot_bar = 0;

static void
println_msg(const char* prefix, id x)
{
  id s = String__from_utf8(prefix);
  s = send(s, ZEFC_SITE("add_o"), ZEFC_SEND0(x, ZEFC_SITE("toString_o")));
  println(s);
}

static id
Foo__foo_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id x = va_arg(ap, id);
  va_end(ap);
  println_msg("hello", x);
  return null_id();
}

static id
Bar__foo_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id x = va_arg(ap, id);
  va_end(ap);
  println_msg("world", x);
  return null_id();
}

static id
Bar__bar_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id x = va_arg(ap, id);
  va_end(ap);
  return Foo__foo_o(self, slot_foo, x);
}

static void
ensure_vtables()
{
  if (slot_foo != 0) {
    return;
  }
  selector_patch(&slot_foo, selector_intern("foo_o1"));
  selector_patch(&slot_bar, selector_intern("bar_o1"));
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
  (void)ZEFC_SEND1(Foo__new(), slot_foo, Int__from_i64(1));
  (void)ZEFC_SEND1(Bar__new(), slot_bar, Int__from_i64(2));
  (void)ZEFC_SEND1(Bar__new(), slot_foo, Int__from_i64(3));
}

} // namespace

void
smoke_super2()
{
  test();
  test();
}

} // namespace smoke
} // namespace zefc
