// Generated from ../zef/tests/testb2.zef (hand-maintained).

#include "zefc/io.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_testb2()
{
  print(String__from_utf8("Hello, "));
  println(String__from_utf8("World!"));
}

} // namespace smoke
} // namespace zefc
