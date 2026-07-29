// Generated from ../zef/tests/test5.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test5()
{
  id x = Int__from_i64(42);
  (void)Int__from_i64(-666);
  println(x);
}

} // namespace smoke
} // namespace zefc
