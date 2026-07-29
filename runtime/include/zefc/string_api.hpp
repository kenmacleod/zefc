#pragma once

#include "zefc/runtime.hpp"

namespace zefc {

const char* String__cstr(id string_obj);
id String__from_utf8(const char* utf8);

} // namespace zefc
