// Generated from ../zef/tests/test42.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test42()
{
  long long x = 42 + ((1 < 2) ? 666 : 1410);
  println(Int__from_i64(x));
}

} // namespace smoke
} // namespace zefc
