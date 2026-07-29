// Generated from ../zef/tests/test3.zef (hand-maintained).

#include <cstdio>
#include <cstdarg>

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

struct TupleFoo_ {
  VTable* isa_;
  id x;
  id y;
  id z;
};

static VTable* TupleFoo_vtable = nullptr;

static id TupleFoo__new_ooo(id, int selector, id x, id y, id z);
static id TupleFoo__add_o(id self, int selector, ...);
static id TupleFoo__toString_o(id self, int selector, ...);

static id
TupleFoo__add_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  TupleFoo_* o = body<TupleFoo_>(other);
  TupleFoo_* s = body<TupleFoo_>(self);
  return TupleFoo__new_ooo(null_id(), 0,
                           send(s->x, zefc_slot_add_o, o->x),
                           send(s->y, zefc_slot_add_o, o->y),
                           send(s->z, zefc_slot_add_o, o->z));
}

static id
TupleFoo__toString_o(id self, int selector, ...)
{
  (void)selector;
  TupleFoo_* s = body<TupleFoo_>(self);
  char buf[64];
  std::snprintf(buf, sizeof(buf), "<%lld,%lld,%lld>",
                Int__to_i64(s->x), Int__to_i64(s->y), Int__to_i64(s->z));
  return String__from_utf8(buf);
}

static id
TupleFoo__new_ooo(id, int selector, id x, id y, id z)
{
  (void)selector;
  TupleFoo_* self = alloc<TupleFoo_>();
  self->isa_ = TupleFoo_vtable;
  self->x = x;
  self->y = y;
  self->z = z;
  return as_id(self);
}

static void
init_tuple_foo_vtable()
{
  if (!TupleFoo_vtable) {
      TupleFoo_vtable = vtable_create();
    }
  vtable_set(TupleFoo_vtable, zefc_slot_add_o, TupleFoo__add_o);
  vtable_set(TupleFoo_vtable, zefc_slot_toString_o, TupleFoo__toString_o);
}

} // namespace

void
smoke_test3()
{
  init_tuple_foo_vtable();
  id a = TupleFoo__new_ooo(null_id(), 0, Int__from_i64(1), Int__from_i64(2), Int__from_i64(3));
  id b = TupleFoo__new_ooo(null_id(), 0, Int__from_i64(4), Int__from_i64(5), Int__from_i64(6));
  println(send(a, zefc_slot_add_o, b));
}

} // namespace smoke
} // namespace zefc
