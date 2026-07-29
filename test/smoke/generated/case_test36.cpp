// Generated from ../zef/tests/test36.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test36()
{
  long long x = 1;
  auto get_x = [&]() -> long long { return x + 42; };
  auto set_x = [&](long long value) { x = value + 666; };
  set_x(get_x() + 67);
  println(Int__from_i64(get_x()));
}

} // namespace smoke
} // namespace zefc
