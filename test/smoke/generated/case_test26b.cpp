// Generated from ../zef/tests/test26b.zef (hand-maintained).
// Structure: times evaluates f without calling — no send; empty stdout.

#include <cstdarg>

#include "zefc/dispatch.hpp"
#include "zefc/runtime.hpp"
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
  // Would print if called; test26b must not call.
  return null_id();
}

static id
make_thunk()
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_call, selector_intern("t26b_call_o"));
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
times(id f, long long n)
{
  long long i = 0;
  while (i < n) {
    (void)f; // Zef: bare `f` — no call
    i += 1;
  }
}

} // namespace

void
smoke_test26b()
{
  times(make_thunk(), 10);
}

} // namespace smoke
} // namespace zefc
