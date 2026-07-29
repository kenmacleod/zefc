// Hand-maintained Double runtime (future: generated from .zefc).

#include <cstdarg>
#include <cmath>
#include <cstdio>

#include "zefc/dispatch.hpp"
#include "zefc/double_api.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"

namespace zefc {
namespace runtime {

struct Double_ {
  VTable* isa_;
  double value;
};

using Double = Double_*;

static VTable* Double_vtable = nullptr;

static Double
Double__from_f64_impl(double value)
{
  Double n = alloc<Double_>();
  n->isa_ = Double_vtable;
  n->value = value;
  return n;
}

static double
Double__to_f64_impl(id obj)
{
  return body<Double_>(obj)->value;
}

static id
Double__toString_o(id self, int selector, ...)
{
  (void)selector;
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.17g", Double__to_f64_impl(self));
  return String__from_utf8(buf);
}

static id
Double__add_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return as_id(Double__from_f64_impl(Double__to_f64_impl(self) + Double__to_f64_impl(other)));
}

static id
Double__sub_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return as_id(Double__from_f64_impl(Double__to_f64_impl(self) - Double__to_f64_impl(other)));
}

static id
Double__mul_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return as_id(Double__from_f64_impl(Double__to_f64_impl(self) * Double__to_f64_impl(other)));
}

static id
Double__div_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id other = va_arg(ap, id);
  va_end(ap);
  return as_id(Double__from_f64_impl(Double__to_f64_impl(self) / Double__to_f64_impl(other)));
}

static id
Double__sqrt_o(id self, int selector, ...)
{
  (void)selector;
  return as_id(Double__from_f64_impl(std::sqrt(Double__to_f64_impl(self))));
}

void
double_runtime_init()
{
  package_register("zefc.runtime.double");
  Double_vtable = vtable_create();
  vtable_set(Double_vtable, selector_intern("toString_o"), Double__toString_o);
  vtable_set(Double_vtable, selector_intern("add_o"), Double__add_o);
  vtable_set(Double_vtable, selector_intern("sub_o"), Double__sub_o);
  vtable_set(Double_vtable, selector_intern("mul_o"), Double__mul_o);
  vtable_set(Double_vtable, selector_intern("div_o"), Double__div_o);
  vtable_set(Double_vtable, selector_intern("sqrt_o"), Double__sqrt_o);
  selector_sites_patch();
}

} // namespace runtime

id
Double__from_f64(double value)
{
  return as_id(runtime::Double__from_f64_impl(value));
}

double
Double__to_f64(id double_obj)
{
  return runtime::Double__to_f64_impl(double_obj);
}

} // namespace zefc
