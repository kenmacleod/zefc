#pragma once

#include "zefc/runtime.hpp"

namespace zefc {

id Double__from_f64(double value);
double Double__to_f64(id double_obj);

// Immediate-Double short-circuit path used by ZEFC_SEND* (Zef Value::add-style).
id zefc_double_send0(id self, int selector);
id zefc_double_send1(id self, int selector, id arg0);

} // namespace zefc
