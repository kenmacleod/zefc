// Generated from ../zef/tests/test23.zef (hand-maintained).

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

struct Bar_ {
  zefc_method* isa_;
  long long a;
  long long b;
  long long c;
};

static zefc_method Bar_vtable[kMaxSelectors];

static id
Bar__call_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id arg = va_arg(ap, id);
  va_end(ap);
  Bar_* c = body<Bar_>(self);
  return Int__from_i64(c->a + c->b + c->c + Int__to_i64(arg));
}

static id
make_bar(long long a, long long b, long long c)
{
  static bool ready = false;
  if (!ready) {
    for (int i = 0; i < kMaxSelectors; ++i) {
      Bar_vtable[i] = doesNotUnderstand;
    }
    vtable_set(Bar_vtable, zefc_slot_add_o, Bar__call_o);
    ready = true;
  }
  Bar_* o = alloc<Bar_>();
  o->isa_ = Bar_vtable;
  o->a = a;
  o->b = b;
  o->c = c;
  return as_id(o);
}

static id
foo(long long b)
{
  long long c = 2;
  id bar = make_bar(1, b, c);
  return send(bar, zefc_slot_add_o, Int__from_i64(42));
}

} // namespace

void
smoke_test23()
{
  println(foo(666));
}

} // namespace smoke
} // namespace zefc
