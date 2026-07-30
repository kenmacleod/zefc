// Generated from ../zef/tests/staticcall7.zef (hand-maintained).
// Structure: Foo() chains nested static call classes four deep.

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Class_ {
  IsaPtr isa_;
  id next;
};

static VTable* vtables[5] = {};
static Class_ nodes[5];
static int slot_call = 0;

static id
Leaf__call_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  return Int__from_i64(42);
}

static id
Chain__call_o(id self, int selector, ...)
{
  (void)selector;
  id next = body<Class_>(self)->next;
  return ZEFC_SEND0(next, slot_call);
}

static void
ensure()
{
  if (slot_call != 0) {
    return;
  }
  selector_patch(&slot_call, selector_intern("sc7_call_o"));
  for (int n = 0; n < 5; ++n) {
    if (!vtables[n]) {
      vtables[n] = vtable_create();
    }
  }
  vtable_set(vtables[4], slot_call, Leaf__call_o);
  for (int n = 0; n < 4; ++n) {
    vtable_set(vtables[n], slot_call, Chain__call_o);
  }
  for (int n = 4; n >= 0; --n) {
    zefc_set_isa(&nodes[n], vtables[n]);
    nodes[n].next = (n == 4) ? null_id() : as_id(&nodes[n + 1]);
  }
}

} // namespace

void
smoke_staticcall7()
{
  ensure();
  println(ZEFC_SEND0(as_id(&nodes[0]), slot_call));
}

} // namespace smoke
} // namespace zefc
