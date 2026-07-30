// Generated from ../zef/tests/staticcall5.zef (hand-maintained).
// Structure: import foo binds Bar; Bar() → call (same as staticcall4 after import).

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct BarClass_ {
  IsaPtr isa_;
};

static VTable* Bar_vtable = nullptr;
static BarClass_ g_Bar;
static int slot_call = 0;

static id
Bar__call_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  return Int__from_i64(42);
}

static void
ensure()
{
  if (slot_call != 0) {
    return;
  }
  selector_patch(&slot_call, selector_intern("sc5_call_o"));
  if (!Bar_vtable) {
      Bar_vtable = vtable_create();
    }
  vtable_set(Bar_vtable, slot_call, Bar__call_o);
  zefc_set_isa(&g_Bar, Bar_vtable);
}

} // namespace

void
smoke_staticcall5()
{
  ensure();
  // import foo → Bar in scope
  id Bar = as_id(&g_Bar);
  println(ZEFC_SEND0(Bar, slot_call));
}

} // namespace smoke
} // namespace zefc
