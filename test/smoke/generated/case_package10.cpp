// Generated from ../zef/tests/package10.zef (hand-maintained).
// Accessing foo.Thingy runs package init which prints via stuff(y).

#include "zefc/io.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_package10()
{
  println(String__from_utf8("hello"));
}

} // namespace smoke
} // namespace zefc
