// Generated from ../zef/tests/int64.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_int64()
{
  long long x = static_cast<long long>(0xa0a0a0a0a0a0a0a0ULL);
  long long b = static_cast<long long>(0x0b0b0b0b0b0b0b0bULL);
  println(Int__from_i64(x));
  println(Int__from_i64(b));
  println(Int__from_i64(x + b));
  println(Int__from_i64(x - b));
  println(Int__from_i64(x * b));
  println(Int__from_i64(x / b));
  println(Int__from_i64(x % b));
}

} // namespace smoke
} // namespace zefc
