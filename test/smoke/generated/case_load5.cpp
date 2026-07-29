// Generated from ../zef/tests/load5.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/module.hpp"
#include "loadable_modules.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_load5()
{
  g_package_foo_f = Int__from_i64(666);
  println(g_package_foo_f);
  module_load("stuff/package.zef");
  println(g_package_foo_f);
}

} // namespace smoke
} // namespace zefc
