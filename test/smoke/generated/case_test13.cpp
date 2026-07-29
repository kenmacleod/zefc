// Generated from ../zef/tests/test13.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test13()
{
  id s = ZEFC_SEND0(Int__from_i64(42), zefc_slot_toString_o);
  s = send(s, zefc_slot_add_o, ZEFC_SEND0(Int__from_i64(666), zefc_slot_toString_o));
  s = send(s, zefc_slot_add_o, String__from_utf8("wat"));
  println(s);
  println(String__from_utf8(""));
}

} // namespace smoke
} // namespace zefc
