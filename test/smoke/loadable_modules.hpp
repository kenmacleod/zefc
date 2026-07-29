#pragma once

#include "zefc/runtime.hpp"

namespace zefc {

// Bindings installed/updated when "stuff/package.zef" is loaded.
extern id g_package_foo_f;
extern id g_package_foo_x;

void register_smoke_loadable_modules();

} // namespace zefc
