// Generated from ../zef/tests/test4.zef (hand-maintained).

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

struct Closure_ {
  VTable* isa_;
  long long captured;
};

static VTable* Closure_vtable = nullptr;

static id
Closure__call_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id arg = va_arg(ap, id);
  va_end(ap);
  Closure_* c = body<Closure_>(self);
  return Int__from_i64(c->captured + Int__to_i64(arg));
}

static id
make_add_closure(long long captured)
{
  static bool vtable_ready = false;
  if (!vtable_ready) {
    if (!Closure_vtable) {
      Closure_vtable = vtable_create();
    }
    vtable_set(Closure_vtable, selector_intern("add_o"), Closure__call_o);
    vtable_ready = true;
  }
  Closure_* c = alloc<Closure_>();
  c->isa_ = Closure_vtable;
  c->captured = captured;
  return as_id(c);
}

static id
foo_fn(id x)
{
  return make_add_closure(Int__to_i64(x));
}

} // namespace

void
smoke_test4()
{
  id partial = foo_fn(Int__from_i64(42));
  id result = send(partial, ZEFC_SITE("add_o"), Int__from_i64(666));
  println(result);
}

} // namespace smoke
} // namespace zefc
