// Generated from ../zef/tests/hex.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_hex()
{
  println(Int__from_i64(0x100));
  println(Int__from_i64(0x10));
  println(Int__from_i64(0xaa));
}

} // namespace smoke
} // namespace zefc
