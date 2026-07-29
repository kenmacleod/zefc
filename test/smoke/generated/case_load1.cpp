// Generated from ../zef/tests/load1.zef (hand-maintained).

#include "zefc/io.hpp"
#include "zefc/module.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_load1()
{
  println(String__from_utf8("hello"));
  module_load("stuff/world.zef");
  println(String__from_utf8("yeah"));
}

} // namespace smoke
} // namespace zefc
