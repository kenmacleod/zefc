// Generated from ../zef/tests/classinfunction.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_classinfunction()
{
  long long arg = 42;
  long long baz = 666 + arg;
  println(Int__from_i64(baz));
}

} // namespace smoke
} // namespace zefc
