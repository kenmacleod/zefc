// Generated from test/smoke/example.zef (hand-maintained).

#include <cstdarg>

#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct HelloFoo_ {
  zefc_method* isa_;
  id x;
};

using HelloFoo = HelloFoo_*;

static zefc_method HelloFoo_vtable[kMaxSelectors];

static id HelloFoo__add_o(id self, int selector, ...);
static id HelloFoo__toString_o(id self, int selector, ...);
static id HelloFoo__new_o(id, int selector, id inX);

static id
HelloFoo__add_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id y = va_arg(ap, id);
  va_end(ap);
  id sx = ZEFC_SEND0(self, zefc_slot_toString_o);
  id sy = ZEFC_SEND0(y, zefc_slot_toString_o);
  id sep = String__from_utf8(" ");
  id mid = send(sx, zefc_slot_add_o, sep);
  id cat = send(mid, zefc_slot_add_o, sy);
  return HelloFoo__new_o(null_id(), 0, cat);
}

static id
HelloFoo__toString_o(id self, int selector, ...)
{
  (void)selector;
  return body<HelloFoo_>(self)->x;
}

id
HelloFoo__new_o(id, int selector, id inX)
{
  (void)selector;
  HelloFoo self = alloc<HelloFoo_>();
  self->isa_ = HelloFoo_vtable;
  self->x = inX;
  return as_id(self);
}

static void
init_hello_foo_vtable()
{
  for (int i = 0; i < kMaxSelectors; ++i) {
    HelloFoo_vtable[i] = doesNotUnderstand;
  }
  vtable_set(HelloFoo_vtable, zefc_slot_toString_o, HelloFoo__toString_o);
  vtable_set(HelloFoo_vtable, zefc_slot_add_o, HelloFoo__add_o);
}

} // namespace

void
smoke_hello()
{
  init_hello_foo_vtable();
  id hello = HelloFoo__new_o(null_id(), 0, String__from_utf8("hello"));
  id world = HelloFoo__new_o(null_id(), 0, String__from_utf8("world"));
  id sum = send(hello, zefc_slot_add_o, world);
  println(sum);
}

} // namespace smoke
} // namespace zefc
