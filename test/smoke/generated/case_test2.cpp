// Generated from ../zef/tests/test2.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"
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
    id line = send(String__from_utf8("x = "), zefc_slot_add_o,
                   ZEFC_SEND0(Int__from_i64(x), zefc_slot_toString_o));
    line = send(line, zefc_slot_add_o, String__from_utf8("\n"));
    string = send(string, zefc_slot_add_o, line);
    x += 1;
  }

  print(string);
}

} // namespace smoke
} // namespace zefc
