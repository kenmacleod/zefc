// Hand-maintained Int runtime (future: generated from .zefc).
// Values that fit in int32 are Zef-style immediates; wider ints are heap IntObjects.

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/known_selectors.hpp"
#include "zefc/runtime.hpp"
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

static id
encode_int32(int value)
{
  // Low-bit tag: objects are even; immediates are odd.
  const uintptr_t bits = (static_cast<uintptr_t>(static_cast<uint32_t>(value)) << 1) | 1u;
  return reinterpret_cast<id>(bits);
}

static long long
decode_int32(id obj)
{
  const uintptr_t bits = reinterpret_cast<uintptr_t>(obj);
  return static_cast<int>(static_cast<uint32_t>(bits >> 1));
}

static Int
Int__heap_from_i64(long long value)
{
  Int n = alloc<Int_>();
  n->isa_ = Int_vtable;
  n->value = value;
  return n;
}

static long long
Int__to_i64_impl(id obj)
{
  if (id_is_int32(obj)) {
    return decode_int32(obj);
  }
  return body<Int_>(obj)->value;
}

static id
Int__from_i64_impl(long long value)
{
  if (static_cast<int>(value) == value) {
    return encode_int32(static_cast<int>(value));
  }
  return as_id(Int__heap_from_i64(value));
}

static id
int_toString(id self)
{
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%lld", Int__to_i64_impl(self));
  return String__from_utf8(buf);
}

static id
Int__toString_o(id self, int selector, ...)
{
  (void)selector;
  return int_toString(self);
}

static id
Int__add_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return Int__from_i64_impl(Int__to_i64_impl(self) + Int__to_i64_impl(other));
}

static id
Int__sub_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return Int__from_i64_impl(Int__to_i64_impl(self) - Int__to_i64_impl(other));
}

static id
Int__mul_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return Int__from_i64_impl(Int__to_i64_impl(self) * Int__to_i64_impl(other));
}

void
int_runtime_init()
{
  package_register("zefc.runtime.int");
  Int_vtable = vtable_create();
  vtable_set(Int_vtable, ZEFC_SEL_toString_o, Int__toString_o);
  vtable_set(Int_vtable, ZEFC_SEL_add_o, Int__add_o);
  vtable_set(Int_vtable, ZEFC_SEL_sub_o, Int__sub_o);
  vtable_set(Int_vtable, ZEFC_SEL_mul_o, Int__mul_o);
  selector_sites_patch();
}

} // namespace runtime

id
Int__from_i64(long long value)
{
  return runtime::Int__from_i64_impl(value);
}

long long
Int__to_i64(id int_obj)
{
  return runtime::Int__to_i64_impl(int_obj);
}

id
zefc_int_send0(id self, int selector)
{
  using namespace runtime;
  if (selector == ZEFC_SEL_toString_o) {
    return int_toString(self);
  }
  std::fprintf(stderr, "doesNotUnderstand: immediate Int selector=%d\n", selector);
  std::exit(1);
}

id
zefc_int_send1(id self, int selector, id arg0)
{
  using namespace runtime;
  const long long a = Int__to_i64_impl(self);
  const long long b = Int__to_i64_impl(arg0);
  if (selector == ZEFC_SEL_add_o) {
    return Int__from_i64_impl(a + b);
  }
  if (selector == ZEFC_SEL_sub_o) {
    return Int__from_i64_impl(a - b);
  }
  if (selector == ZEFC_SEL_mul_o) {
    return Int__from_i64_impl(a * b);
  }
  std::fprintf(stderr, "doesNotUnderstand: immediate Int selector=%d\n", selector);
  std::exit(1);
}

} // namespace zefc
