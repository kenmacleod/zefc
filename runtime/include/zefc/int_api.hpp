#pragma once

#include "zefc/runtime.hpp"

namespace zefc {

id Int__from_i64(long long value);
long long Int__to_i64(id int_obj);

// True for immediate int32 or heap Int (values outside int32 range).
bool id_is_int(id v);

// Immediate-Int32 short-circuit path used by ZEFC_SEND* (Zef Value::add-style).
id zefc_int_send0(id self, int selector);
id zefc_int_send1(id self, int selector, id arg0);

} // namespace zefc
