// Generated from ../zef/tests/precedence.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

static id
int_lit(long long n)
{
  return Int__from_i64(n);
}

void
smoke_precedence()
{
  id two = int_lit(2);
  id three = int_lit(3);
  id four = int_lit(4);
  id five = int_lit(5);
  id seven = int_lit(7);

  println(send(two, ZEFC_SITE("add_o"), send(three, ZEFC_SITE("mul_o"), four)));
  println(send(send(two, ZEFC_SITE("add_o"), three), ZEFC_SITE("mul_o"), four));
  println(send(five, ZEFC_SITE("add_o"), send(int_lit(0), ZEFC_SITE("sub_o"), seven)));
}

} // namespace smoke
} // namespace zefc
