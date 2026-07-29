// Generated from ../zef/tests/test27.zef (hand-maintained).
// Structure: foo(f) sends zero-arg call to multi-statement thunk.

#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Thunk_ {
  zefc_method* isa_;
};

static zefc_method Thunk_vtable[kMaxSelectors];
static int slot_call = 0;

static id
Thunk__call_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  println(String__from_utf8("hello"));
  println(String__from_utf8("world"));
  return null_id();
}

static id
make_thunk()
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_call, selector_intern("t27_call_o"));
    for (int i = 0; i < kMaxSelectors; ++i) {
      Thunk_vtable[i] = doesNotUnderstand;
    }
    vtable_set(Thunk_vtable, slot_call, Thunk__call_o);
    ready = true;
  }
  Thunk_* t = alloc<Thunk_>();
  t->isa_ = Thunk_vtable;
  return as_id(t);
}

static void
foo(id f)
{
  (void)ZEFC_SEND0(f, slot_call);
}

} // namespace

void
smoke_test27()
{
  foo(make_thunk());
}

} // namespace smoke
} // namespace zefc
