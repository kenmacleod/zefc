// Hand-maintained Array runtime (future: generated from .zefc).

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#include "zefc/array_api.hpp"
#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"

namespace zefc {
namespace runtime {

struct Array_ {
  zefc_method* isa_;
  std::vector<id> elems;
};

static zefc_method Array_vtable[kMaxSelectors];

static id Array__toString_o(id self, int selector, ...);
static id Array__push_o(id self, int selector, ...);
static id Array__GET_i(id self, int selector, ...);
static id Array__mul_PUT_i(id self, int selector, ...);

static id
Array__toString_o(id self, int selector, ...)
{
  (void)selector;
  Array_* arr = body<Array_>(self);
  std::string out = "[";
  for (size_t i = 0; i < arr->elems.size(); ++i) {
    if (i > 0) {
      out += ',';
    }
    id s = ZEFC_SEND0(arr->elems[i], zefc_slot_toString_o);
    out += String__cstr(s);
  }
  out += ']';
  return String__from_utf8(out.c_str());
}

static id
Array__push_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id value = va_arg(ap, id);
  va_end(ap);
  body<Array_>(self)->elems.push_back(value);
  return self;
}

static id
Array__GET_i(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id index_obj = va_arg(ap, id);
  va_end(ap);
  int index = static_cast<int>(Int__to_i64(index_obj));
  return body<Array_>(self)->elems.at(static_cast<size_t>(index));
}

static id
Array__mul_PUT_i(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id index_obj = va_arg(ap, id);
  id factor_obj = va_arg(ap, id);
  va_end(ap);
  int index = static_cast<int>(Int__to_i64(index_obj));
  long long factor = Int__to_i64(factor_obj);
  Array_* arr = body<Array_>(self);
  id& slot = arr->elems.at(static_cast<size_t>(index));
  slot = Int__from_i64(Int__to_i64(slot) * factor);
  return factor_obj;
}

static id
Array__from_ints_impl(std::initializer_list<long long> values)
{
  Array_* arr = alloc<Array_>();
  arr->isa_ = Array_vtable;
  for (long long v : values) {
    arr->elems.push_back(Int__from_i64(v));
  }
  return as_id(arr);
}

void
array_runtime_init()
{
  package_register("zefc.runtime.array");
  for (int i = 0; i < kMaxSelectors; ++i) {
    Array_vtable[i] = doesNotUnderstand;
  }
  if (zefc_slot_toString_o == 0) {
    selector_patch(&zefc_slot_toString_o, selector_intern("toString_o"));
  }
  selector_patch(&zefc_slot_push_o, selector_intern("push_o"));
  selector_patch(&zefc_slot_GET_i, selector_intern("GET_i"));
  selector_patch(&zefc_slot_mul_PUT_i, selector_intern("mul_PUT_i"));
  vtable_set(Array_vtable, zefc_slot_toString_o, Array__toString_o);
  vtable_set(Array_vtable, zefc_slot_push_o, Array__push_o);
  vtable_set(Array_vtable, zefc_slot_GET_i, Array__GET_i);
  vtable_set(Array_vtable, zefc_slot_mul_PUT_i, Array__mul_PUT_i);
}

} // namespace runtime

id
Array__from_ints(std::initializer_list<long long> values)
{
  return runtime::Array__from_ints_impl(values);
}

id
Array__push(id array, id value)
{
  return ZEFC_SEND1(array, zefc_slot_push_o, value);
}

id
Array__at(id array, int index)
{
  return ZEFC_SEND1(array, zefc_slot_GET_i, Int__from_i64(index));
}

void
Array__mul_assign_at(id array, int index, long long factor)
{
  (void)ZEFC_SEND2(array, zefc_slot_mul_PUT_i, Int__from_i64(index), Int__from_i64(factor));
}

} // namespace zefc
