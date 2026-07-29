// Generated from ../zef/tests/test26.zef (hand-maintained).
// Structure: times(f, n) sends zero-arg call to closure f.

#include <cstdarg>

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Thunk_ {
  VTable* isa_;
};

static VTable* Thunk_vtable = nullptr;
static int slot_call = 0;

static id
Thunk__call_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  println(String__from_utf8("hello"));
  return null_id();
}

static id
make_thunk()
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_call, selector_intern("t26_call_o"));
    if (!Thunk_vtable) {
      Thunk_vtable = vtable_create();
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
    (void)ZEFC_SEND0(f, slot_call);
    i += 1;
  }
}

} // namespace

void
smoke_test26()
{
  times(make_thunk(), 10);
}

} // namespace smoke
} // namespace zefc
