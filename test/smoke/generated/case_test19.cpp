// Generated from ../zef/tests/test19.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test19()
{
  long long x = 1;
  while (1) {
    if (x > 5) {
      println(Int__from_i64(x));
      return;
    }
    x += 1;
  }
}

} // namespace smoke
} // namespace zefc
