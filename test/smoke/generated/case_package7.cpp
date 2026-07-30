// Generated from ../zef/tests/package7.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Thingy_ {
  IsaPtr isa_;
  id x;
};

static VTable* Thingy_vtable = nullptr;

static id
Thingy__toString_o(id self, int selector, ...)
{
  (void)selector;
  return body<Thingy_>(self)->x;
}

static id
Thingy__new(id x)
{
  static bool ready = false;
  if (!ready) {
    if (!Thingy_vtable) {
      Thingy_vtable = vtable_create();
    }
    vtable_set(Thingy_vtable, selector_intern("toString_o"), Thingy__toString_o);
    ready = true;
  }
  Thingy_* t = alloc<Thingy_>();
  zefc_set_isa(t, Thingy_vtable);
  t->x = x;
  return as_id(t);
}

} // namespace

void
smoke_package7()
{
  id y = Thingy__new(String__from_utf8("hello"));
  println(body<Thingy_>(y)->x);
}

} // namespace smoke
} // namespace zefc
