// Generated from ../zef/tests/staticcall4.zef (hand-maintained).
// Structure: package foo { class Bar { static call } }; foo.Bar()

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Pkg_ {
  zefc_method* isa_;
  id Bar;
};

struct BarClass_ {
  zefc_method* isa_;
};

static zefc_method Pkg_vtable[kMaxSelectors];
static zefc_method Bar_vtable[kMaxSelectors];
static Pkg_ g_foo;
static BarClass_ g_Bar;
static int slot_Bar = 0;
static int slot_call = 0;

static id
Bar__call_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  return Int__from_i64(42);
}

static id
Pkg__Bar_o(id self, int selector, ...)
{
  (void)selector;
  return body<Pkg_>(self)->Bar;
}

static void
ensure()
{
  if (slot_call != 0) {
    return;
  }
  selector_patch(&slot_Bar, selector_intern("sc4_Bar_o"));
  selector_patch(&slot_call, selector_intern("sc4_call_o"));
  for (int i = 0; i < kMaxSelectors; ++i) {
    Pkg_vtable[i] = doesNotUnderstand;
    Bar_vtable[i] = doesNotUnderstand;
  }
  vtable_set(Bar_vtable, slot_call, Bar__call_o);
  vtable_set(Pkg_vtable, slot_Bar, Pkg__Bar_o);
  g_Bar.isa_ = Bar_vtable;
  g_foo.isa_ = Pkg_vtable;
  g_foo.Bar = as_id(&g_Bar);
}

} // namespace

void
smoke_staticcall4()
{
  ensure();
  id Bar = ZEFC_SEND0(as_id(&g_foo), slot_Bar);
  println(ZEFC_SEND0(Bar, slot_call));
}

} // namespace smoke
} // namespace zefc
