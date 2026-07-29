// Generated from ../zef/tests/test43.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

static long long
foo(long long x)
{
  if (x < 42) {
    return 0;
  }
  return 666;
}

} // namespace

void
smoke_test43()
{
  println(Int__from_i64(foo(1)));
  println(Int__from_i64(foo(100)));
}

} // namespace smoke
} // namespace zefc
