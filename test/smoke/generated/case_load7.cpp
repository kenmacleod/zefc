// Generated from ../zef/tests/load7.zef (hand-maintained).
// Block with `my y` is a new scope; import(foo) binds f/x from the package.

#include "zefc/int_api.hpp"
#include "zefc/module.hpp"
#include "loadable_modules.hpp"
#include "smoke_cases.hpp"
#include "smoke_helpers.hpp"

namespace zefc {
namespace smoke {

void
smoke_load7()
{
  long long outer_f = 666;
  long long outer_x = 1410;
  {
    long long f = outer_f;
    long long x = outer_x;
    long long y = 0;
    (void)y;
    println_fx("(1)", f, x);
    module_load("stuff/package.zef");
    println_fx("(2)", f, x);
    f = Int__to_i64(g_package_foo_f);
    x = Int__to_i64(g_package_foo_x);
    println_fx("(3)", f, x);
  }
}

} // namespace smoke
} // namespace zefc
