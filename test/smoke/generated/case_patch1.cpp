// Acceptance case for dispatch ABI step A:
// Module A then B; B introduces a new selector (vtable growth); sends use
// ZEFC_SITE patch cells (not shared zefc_slot_* globals).

#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/module.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct PingPong_ {
  VTable* isa_;
};

static VTable* PingPong_vtable = nullptr;

static id
PingPong__ping_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  println(String__from_utf8("ping"));
  return null_id();
}

static id
PingPong__pong_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  println(String__from_utf8("pong"));
  return null_id();
}

static void
module_a()
{
  if (!PingPong_vtable) {
    PingPong_vtable = vtable_create();
  }
  const int ping = selector_intern("ping_o");
  vtable_set(PingPong_vtable, ping, PingPong__ping_o);
}

static void
module_b()
{
  // New selector forces capacity growth on all registered vtables.
  const int pong = selector_intern("pong_o");
  vtable_set(PingPong_vtable, pong, PingPong__pong_o);
}

static id
make_obj()
{
  PingPong_* o = alloc<PingPong_>();
  o->isa_ = PingPong_vtable;
  return as_id(o);
}

} // namespace

void
smoke_patch1()
{
  module_register("patch1/a", module_a);
  module_register("patch1/b", module_b);

  // Call-site cells (one each); lazy-intern on first use — not shared globals.
  const int ping_site = ZEFC_SITE("ping_o");
  const int pong_site = ZEFC_SITE("pong_o");

  module_load("patch1/a");
  id obj = make_obj();
  (void)ZEFC_SEND0(obj, ping_site);

  module_load("patch1/b");
  (void)ZEFC_SEND0(obj, ping_site);
  (void)ZEFC_SEND0(obj, pong_site);
}

} // namespace smoke
} // namespace zefc
