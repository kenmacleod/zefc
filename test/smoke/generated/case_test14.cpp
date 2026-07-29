// Generated from ../zef/tests/test14.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test14()
{
  long long x = 1;
  while (1) {
    if (x > 5) {
      break;
    }
    x += 1;
  }
  println(Int__from_i64(x));
}

} // namespace smoke
} // namespace zefc
