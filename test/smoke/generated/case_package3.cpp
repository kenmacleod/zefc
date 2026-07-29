// Generated from ../zef/tests/package3.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_package3()
{
  println(send(String__from_utf8("thingy "), zefc_slot_add_o, String__from_utf8("hello")));
  println(send(String__from_utf8("stuff "), zefc_slot_add_o, String__from_utf8("world")));
}

} // namespace smoke
} // namespace zefc
