// Generated from ../zef/tests/load10.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/module.hpp"
#include "loadable_modules.hpp"
#include "smoke_cases.hpp"
#include "smoke_helpers.hpp"

namespace zefc {
namespace smoke {

void
smoke_load10()
{
  long long outer_f = 666;
  long long outer_x = 1410;
  auto stuff = [&]() {
    long long f = outer_f;
    long long x = outer_x;
    println_fx("(1)", f, x);
    module_load("stuff/package.zef");
    println_fx("(2)", f, x);
    f = Int__to_i64(g_package_foo_f);
    x = Int__to_i64(g_package_foo_x);
    println_fx("(3)", f, x);
  };
  stuff();
}

} // namespace smoke
} // namespace zefc
