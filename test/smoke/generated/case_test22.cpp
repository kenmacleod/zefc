// Generated from ../zef/tests/test22.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test22()
{
  long long x_ = 0;
  auto get_x = [&]() -> long long { return x_ + 42; };
  auto set_x = [&](long long value) { x_ = value + 666; };
  set_x(1410);
  println(Int__from_i64(get_x()));
}

} // namespace smoke
} // namespace zefc
