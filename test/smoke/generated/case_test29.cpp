// Generated from ../zef/tests/test29.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test29()
{
  println(Int__from_i64(6 * 7));
  println(send(Int__from_i64(6), ZEFC_SITE("mul_o"), Int__from_i64(7)));
}

} // namespace smoke
} // namespace zefc
