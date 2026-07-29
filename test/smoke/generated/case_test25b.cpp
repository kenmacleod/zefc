// Generated from ../zef/tests/test25b.zef (hand-maintained).

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
  zefc_method* isa_;
  long long a;
  long long b;
};

struct Closure0_ {
  zefc_method* isa_;
  long long a;
};

static zefc_method Closure1_vtable[kMaxSelectors];
static zefc_method Closure0_vtable[kMaxSelectors];

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
    for (int i = 0; i < kMaxSelectors; ++i) {
      Closure1_vtable[i] = doesNotUnderstand;
    }
    vtable_set(Closure1_vtable, zefc_slot_add_o, Closure1__call_o);
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
    for (int i = 0; i < kMaxSelectors; ++i) {
      Closure0_vtable[i] = doesNotUnderstand;
    }
    vtable_set(Closure0_vtable, zefc_slot_add_o, Closure0__call_o);
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
smoke_test25b()
{
  id g = f_fn(Int__from_i64(1));
  id h = send(g, zefc_slot_add_o, Int__from_i64(2));
  println(send(h, zefc_slot_add_o, Int__from_i64(3)));
}

} // namespace smoke
} // namespace zefc
