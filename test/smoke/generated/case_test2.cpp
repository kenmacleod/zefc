// Generated from ../zef/tests/test2.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test2()
{
  long long x = 1;
  id string = String__from_utf8("");

  while (x <= 10) {
    id line = send(String__from_utf8("x = "), ZEFC_SITE("add_o"),
                   ZEFC_SEND0(Int__from_i64(x), ZEFC_SITE("toString_o")));
    line = send(line, ZEFC_SITE("add_o"), String__from_utf8("\n"));
    string = send(string, ZEFC_SITE("add_o"), line);
    x += 1;
  }

  print(string);
}

} // namespace smoke
} // namespace zefc
