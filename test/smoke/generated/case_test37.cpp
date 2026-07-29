// Generated from ../zef/tests/test37.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test37()
{
  id s = String__from_utf8("Hello ");
  s = send(s, ZEFC_SITE("add_o"), ZEFC_SEND0(Int__from_i64(666), ZEFC_SITE("toString_o")));
  s = send(s, ZEFC_SITE("add_o"), String__from_utf8(" World"));
  println(s);
}

} // namespace smoke
} // namespace zefc
