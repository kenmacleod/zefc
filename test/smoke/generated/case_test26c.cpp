// Generated from ../zef/tests/test26c.zef (hand-maintained).
// Structure: times(f, n) sends f(i) each iteration.

#include <cstdarg>

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct Fn_ {
  VTable* isa_;
};

static VTable* Fn_vtable = nullptr;
static int slot_call = 0;

static id
Fn__call_o(id self, int selector, ...)
{
  (void)self;
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id i = va_arg(ap, id);
  va_end(ap);
  println(i);
  return null_id();
}

static id
make_fn()
{
  static bool ready = false;
  if (!ready) {
    selector_patch(&slot_call, selector_intern("t26c_call_o"));
    if (!Fn_vtable) {
      Fn_vtable = vtable_create();
    }
    vtable_set(Fn_vtable, slot_call, Fn__call_o);
    ready = true;
  }
  Fn_* f = alloc<Fn_>();
  f->isa_ = Fn_vtable;
  return as_id(f);
}

static void
times(id f, long long n)
{
  long long i = 0;
  while (i < n) {
    (void)send(f, slot_call, Int__from_i64(i));
    i += 1;
  }
}

} // namespace

void
smoke_test26c()
{
  times(make_fn(), 10);
}

} // namespace smoke
} // namespace zefc
