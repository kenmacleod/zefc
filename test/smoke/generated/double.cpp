// Hand-maintained Double runtime (future: generated from .zefc).
// Doubles are Zef-style NaN-box immediates (no heap). Arithmetic short-circuits
// in ZEFC_SEND* via zefc_double_send*; compares use closed-world ZEFC_SEL_* imms.

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "zefc/dispatch.hpp"
#include "zefc/double_api.hpp"
#include "zefc/known_selectors.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"

namespace zefc {
namespace runtime {

static VTable* Double_vtable = nullptr;

static id
encode_double(double value)
{
  uintptr_t bits = 0;
  static_assert(sizeof(bits) == sizeof(value), "double/uintptr_t size");
  std::memcpy(&bits, &value, sizeof(bits));
  bits += kDoubleTagMagic;
  if (bits < kDoubleTagMagic) {
    std::fprintf(stderr, "Double: signaling NaN\n");
    std::exit(1);
  }
  return reinterpret_cast<id>(bits);
}

static double
decode_double(id obj)
{
  uintptr_t bits = reinterpret_cast<uintptr_t>(obj) - kDoubleTagMagic;
  double value = 0;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

static id
double_toString(id self)
{
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.17g", decode_double(self));
  return String__from_utf8(buf);
}

void
double_runtime_init()
{
  package_register("zefc.runtime.double");
  // Selectors reserved in known_selectors_init; vtable is a namespace placeholder.
  Double_vtable = vtable_create();
  selector_sites_patch();
}

} // namespace runtime

id
Double__from_f64(double value)
{
  return runtime::encode_double(value);
}

double
Double__to_f64(id double_obj)
{
  if (!id_is_double(double_obj)) {
    std::fprintf(stderr, "Double__to_f64: expected immediate double\n");
    std::exit(1);
  }
  return runtime::decode_double(double_obj);
}

id
zefc_double_send0(id self, int selector)
{
  using namespace runtime;
  if (selector == ZEFC_SEL_toString_o) {
    return double_toString(self);
  }
  if (selector == ZEFC_SEL_sqrt_o) {
    return encode_double(std::sqrt(decode_double(self)));
  }
  std::fprintf(stderr, "doesNotUnderstand: immediate Double selector=%d\n", selector);
  std::exit(1);
}

id
zefc_double_send1(id self, int selector, id arg0)
{
  using namespace runtime;
  if (!id_is_double(arg0)) {
    std::fprintf(stderr, "Double binary op: expected immediate double arg\n");
    std::exit(1);
  }
  const double a = decode_double(self);
  const double b = decode_double(arg0);
  if (selector == ZEFC_SEL_add_o) {
    return encode_double(a + b);
  }
  if (selector == ZEFC_SEL_sub_o) {
    return encode_double(a - b);
  }
  if (selector == ZEFC_SEL_mul_o) {
    return encode_double(a * b);
  }
  if (selector == ZEFC_SEL_div_o) {
    return encode_double(a / b);
  }
  std::fprintf(stderr, "doesNotUnderstand: immediate Double selector=%d\n", selector);
  std::exit(1);
}

} // namespace zefc
