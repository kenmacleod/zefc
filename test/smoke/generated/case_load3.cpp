// Generated from ../zef/tests/load3.zef (hand-maintained).

#include "zefc/error.hpp"
#include "zefc/module.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_load3()
{
  module_load("stuff/fn.zef");
  zefc_error("cannot resolve get (call with no arguments) named f");
}

} // namespace smoke
} // namespace zefc
