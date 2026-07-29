// Generated from ../zef/tests/package1.zef (hand-maintained).

#include "zefc/io.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_package1()
{
  // package foo { fn thingy println("hello") }; foo.thingy
  println(String__from_utf8("hello"));
}

} // namespace smoke
} // namespace zefc
