// Generated from ../zef/tests/package2.zef (hand-maintained).

#include "zefc/io.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_package2()
{
  // package foo { package bar { fn thingy println("hello") } }; foo.bar.thingy
  println(String__from_utf8("hello"));
}

} // namespace smoke
} // namespace zefc
