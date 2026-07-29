// Hand-maintained Int runtime (future: generated from .zefc).

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"

namespace zefc {
namespace runtime {

struct Int_ {
  VTable* isa_;
  long long value;
};

using Int = Int_*;

static VTable* Int_vtable = nullptr;

static id Int__toString_o(id self, int selector, ...);
static id Int__add_o(id self, int selector, ...);
static id Int__sub_o(id self, int selector, ...);
static id Int__mul_o(id self, int selector, ...);

static Int
Int__from_i64_impl(long long value)
{
  Int n = alloc<Int_>();
  n->isa_ = Int_vtable;
  n->value = value;
  return n;
}

static long long
Int__to_i64_impl(id obj)
{
  return body<Int_>(obj)->value;
}

static id
Int__toString_o(id self, int selector, ...)
{
  (void)selector;
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(body<Int_>(self)->value));
  return String__from_utf8(buf);
}

static id
Int__add_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return as_id(Int__from_i64_impl(Int__to_i64_impl(self) + Int__to_i64_impl(other)));
}

static id
Int__sub_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return as_id(Int__from_i64_impl(Int__to_i64_impl(self) - Int__to_i64_impl(other)));
}

static id
Int__mul_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return as_id(Int__from_i64_impl(Int__to_i64_impl(self) * Int__to_i64_impl(other)));
}

void
int_runtime_init()
{
  package_register("zefc.runtime.int");
  Int_vtable = vtable_create();
  vtable_set(Int_vtable, selector_intern("toString_o"), Int__toString_o);
  vtable_set(Int_vtable, selector_intern("add_o"), Int__add_o);
  vtable_set(Int_vtable, selector_intern("sub_o"), Int__sub_o);
  vtable_set(Int_vtable, selector_intern("mul_o"), Int__mul_o);
  selector_sites_patch();
}

} // namespace runtime

id
Int__from_i64(long long value)
{
  return as_id(runtime::Int__from_i64_impl(value));
}

long long
Int__to_i64(id int_obj)
{
  return runtime::Int__to_i64_impl(int_obj);
}

} // namespace zefc
