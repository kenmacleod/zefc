// Generated from ../zef/tests/load6.zef (hand-maintained).
// Block with no definitions shares the outer context; import(foo) does not
// overwrite the outer f/x (matches Zef expected output).

#include "zefc/module.hpp"
#include "loadable_modules.hpp"
#include "smoke_cases.hpp"
#include "smoke_helpers.hpp"

namespace zefc {
namespace smoke {

void
smoke_load6()
{
  long long f = 666;
  long long x = 1410;
  println_fx("(1)", f, x);
  module_load("stuff/package.zef");
  println_fx("(2)", f, x);
  // import(foo) — shared outer context keeps existing f/x
  println_fx("(3)", f, x);
}

} // namespace smoke
} // namespace zefc
