// Generated from ../zef/tests/test26.zef (hand-maintained).

#include "zefc/io.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test26()
{
  long long i = 0;
  while (i < 10) {
    println(String__from_utf8("hello"));
    i += 1;
  }
}

} // namespace smoke
} // namespace zefc
