// Generated from ../zef/tests/test25.zef (hand-maintained).

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

struct Closure1_ {
  VTable* isa_;
  long long a;
  long long b;
};

struct Closure0_ {
  VTable* isa_;
  long long a;
};

static VTable* Closure1_vtable = nullptr;
static VTable* Closure0_vtable = nullptr;

static id
Closure1__call_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id arg = va_arg(ap, id);
  va_end(ap);
  Closure1_* c = body<Closure1_>(self);
  return Int__from_i64(c->a + c->b + Int__to_i64(arg));
}

static id
make_closure1(long long a, long long b)
{
  static bool ready = false;
  if (!ready) {
    if (!Closure1_vtable) {
      Closure1_vtable = vtable_create();
    }
    vtable_set(Closure1_vtable, selector_intern("add_o"), Closure1__call_o);
    ready = true;
  }
  Closure1_* c = alloc<Closure1_>();
  c->isa_ = Closure1_vtable;
  c->a = a;
  c->b = b;
  return as_id(c);
}

static id
Closure0__call_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id arg = va_arg(ap, id);
  va_end(ap);
  Closure0_* c = body<Closure0_>(self);
  return make_closure1(c->a, Int__to_i64(arg));
}

static id
make_closure0(long long a)
{
  static bool ready = false;
  if (!ready) {
    if (!Closure0_vtable) {
      Closure0_vtable = vtable_create();
    }
    vtable_set(Closure0_vtable, selector_intern("add_o"), Closure0__call_o);
    ready = true;
  }
  Closure0_* c = alloc<Closure0_>();
  c->isa_ = Closure0_vtable;
  c->a = a;
  return as_id(c);
}

static id
f_fn(id a)
{
  return make_closure0(Int__to_i64(a));
}

} // namespace

void
smoke_test25()
{
  id g = f_fn(Int__from_i64(42));
  id h = send(g, ZEFC_SITE("add_o"), Int__from_i64(666));
  id result = send(h, ZEFC_SITE("add_o"), Int__from_i64(1410));
  println(result);
}

} // namespace smoke
} // namespace zefc
