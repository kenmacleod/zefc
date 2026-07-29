// Generated from ../zef/tests/test33.zef (hand-maintained).

#include "zefc/io.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test33()
{
  if (4 < 5) {
    println(String__from_utf8("yes"));
  } else {
    println(String__from_utf8("no"));
  }
}

} // namespace smoke
} // namespace zefc
