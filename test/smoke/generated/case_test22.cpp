// Generated from ../zef/tests/test22.zef (hand-maintained).
// Structure: scope object with x get / set_x; assignment and read via sends.

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

struct Scope_ {
  IsaPtr isa_;
  id x_;
};

static VTable* Scope_vtable = nullptr;
static int slot_x = 0;
static int slot_set_x = 0;

static id
Scope__x_o(id self, int selector, ...)
{
  (void)selector;
  return send(body<Scope_>(self)->x_, ZEFC_SITE("add_o"), Int__from_i64(42));
}

static id
Scope__set_x_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id value = va_arg(ap, id);
  va_end(ap);
  body<Scope_>(self)->x_ = send(value, ZEFC_SITE("add_o"), Int__from_i64(666));
  return null_id();
}

static id
Scope__new()
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_x, selector_intern("t22_x_o"));
    selector_patch(&slot_set_x, selector_intern("t22_set_x_o"));
    if (!Scope_vtable) {
      Scope_vtable = vtable_create();
    }
    vtable_set(Scope_vtable, slot_x, Scope__x_o);
    vtable_set(Scope_vtable, slot_set_x, Scope__set_x_o);
    ready = true;
  }
  Scope_* s = alloc<Scope_>();
  zefc_set_isa(s, Scope_vtable);
  s->x_ = Int__from_i64(0);
  return as_id(s);
}

} // namespace

void
smoke_test22()
{
  id scope = Scope__new();
  (void)ZEFC_SEND1(scope, slot_set_x, Int__from_i64(1410));
  println(ZEFC_SEND0(scope, slot_x));
}

} // namespace smoke
} // namespace zefc
