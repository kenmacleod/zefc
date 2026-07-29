// Generated from ../zef/tests/testb.zef (hand-maintained).

#include "zefc/io.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_testb()
{
  print(String__from_utf8("Hello, "));
  println(String__from_utf8("World!"));
}

} // namespace smoke
} // namespace zefc
