// Generated from ../zef/tests/test15.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test15()
{
  long long x = 1;
  while (1) {
    if (x <= 5) {
      x += 1;
      continue;
    }
    break;
  }
  println(Int__from_i64(x));
}

} // namespace smoke
} // namespace zefc
