// Generated from ../zef/tests/test34.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test34()
{
  long long i = 0;
  while (1) {
    i += 1;
    if (i > 6) {
      break;
    }
  }
  println(Int__from_i64(i));
}

} // namespace smoke
} // namespace zefc
