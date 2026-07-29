// Generated from ../zef/tests/test26c.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test26c()
{
  long long i = 0;
  while (i < 10) {
    println(Int__from_i64(i));
    i += 1;
  }
}

} // namespace smoke
} // namespace zefc
