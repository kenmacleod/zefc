// Generated from ../zef/tests/test35.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test35()
{
  long long i = 0;
  while (1) {
    i += 1;
    if (i & 1) {
      continue;
    }
    println(Int__from_i64(i));
    if (i > 6) {
      break;
    }
  }
}

} // namespace smoke
} // namespace zefc
