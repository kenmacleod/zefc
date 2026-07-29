// Generated from ../zef/tests/accessors.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_accessors()
{
  long long a = 1;
  long long b = 2;
  long long c = 3;
  long long d = 4;
  println(Int__from_i64(a));
  println(Int__from_i64(b));
  b = 42;
  println(Int__from_i64(b));
  println(Int__from_i64(c));
  println(Int__from_i64(d));
  d = 666;
  println(Int__from_i64(d));
}

} // namespace smoke
} // namespace zefc
