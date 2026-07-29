// Generated from ../zef/tests/test30.zef (hand-maintained).
// Structure: Foo instances; identity == via pointer compare (Int 0/1).

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
  VTable* isa_;
};

static VTable* Foo_vtable = nullptr;
static int slot_eq = 0;

static id
Foo__eq_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return Int__from_i64(self == other ? 1 : 0);
}

static id
Foo__new()
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_eq, selector_intern("eq_o"));
    if (!Foo_vtable) {
      Foo_vtable = vtable_create();
    }
    vtable_set(Foo_vtable, slot_eq, Foo__eq_o);
    ready = true;
  }
  Foo_* o = alloc<Foo_>();
  o->isa_ = Foo_vtable;
  return as_id(o);
}

} // namespace

void
smoke_test30()
{
  id f = Foo__new();
  println(send(f, slot_eq, f));
  println(send(f, slot_eq, Foo__new()));
}

} // namespace smoke
} // namespace zefc
