// Generated from ../zef/tests/load4.zef (hand-maintained).

#include "zefc/io.hpp"
#include "zefc/module.hpp"
#include "loadable_modules.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_load4()
{
  module_load("stuff/package.zef");
  println(g_package_foo_f);
  println(g_package_foo_x);
}

} // namespace smoke
} // namespace zefc
