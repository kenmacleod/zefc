// Generated from ../zef/tests/load2.zef (hand-maintained).

#include "zefc/error.hpp"
#include "zefc/module.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_load2()
{
  module_load("stuff/var.zef");
  zefc_error("cannot resolve get (call with no arguments) named x");
}

} // namespace smoke
} // namespace zefc
