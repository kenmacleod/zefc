// Generated from ../zef/tests/test36.zef (hand-maintained).
// Structure: Thingy field get (`x`) / set (`set_x`) via vtable sends; `t.x += 67`.

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

struct Thingy_ {
  VTable* isa_;
  id x;
};

static VTable* Thingy_vtable = nullptr;
static int slot_x = 0;
static int slot_set_x = 0;

static id
Thingy__x_o(id self, int selector, ...)
{
  (void)selector;
  Thingy_* t = body<Thingy_>(self);
  return send(t->x, zefc_slot_add_o, Int__from_i64(42));
}

static id
Thingy__set_x_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id value = va_arg(ap, id);
  va_end(ap);
  Thingy_* t = body<Thingy_>(self);
  t->x = send(value, zefc_slot_add_o, Int__from_i64(666));
  return null_id();
}

static id
Thingy__new(id inX)
{
  static bool ready = false;
  if (!ready) {
    if (slot_x == 0) {
      selector_patch(&slot_x, selector_intern("x_o"));
      selector_patch(&slot_set_x, selector_intern("set_x_o"));
    }
    if (!Thingy_vtable) {
      Thingy_vtable = vtable_create();
    }
    vtable_set(Thingy_vtable, slot_x, Thingy__x_o);
    vtable_set(Thingy_vtable, slot_set_x, Thingy__set_x_o);
    ready = true;
  }
  Thingy_* t = alloc<Thingy_>();
  t->isa_ = Thingy_vtable;
  t->x = inX;
  return as_id(t);
}

} // namespace

void
smoke_test36()
{
  id t = Thingy__new(Int__from_i64(1));
  // t.x += 67  →  set_x(get_x() + 67)
  id cur = ZEFC_SEND0(t, slot_x);
  id summed = send(cur, zefc_slot_add_o, Int__from_i64(67));
  (void)ZEFC_SEND1(t, slot_set_x, summed);
  println(ZEFC_SEND0(t, slot_x));
}

} // namespace smoke
} // namespace zefc
