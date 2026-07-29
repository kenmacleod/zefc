// Generated from ../zef/tests/accessors2b.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_accessors2b()
{
  long long a = 1;
  long long b = 2;
  println(Int__from_i64(a));
  println(Int__from_i64(b));
  b = 42;
  println(Int__from_i64(b));
}

} // namespace smoke
} // namespace zefc
