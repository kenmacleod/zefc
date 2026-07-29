// Generated from ../zef/tests/test6.zef (hand-maintained).

#include <cstdio>

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
smoke_test6()
{
  long long thingy = 676;
  id stuff = String__from_utf8("hello");
  id whatever = String__from_utf8("666");
  long long blah = 671;

  auto foo_doShit = [&]() {
    thingy += 1;
    stuff = send(stuff, zefc_slot_add_o, String__from_utf8("x"));
  };

  auto bar_doShit = [&]() {
    foo_doShit();
    whatever = send(whatever, zefc_slot_add_o, String__from_utf8("y"));
    blah += 2;
  };

  bar_doShit();
  bar_doShit();

  char buf[128];
  std::snprintf(buf, sizeof(buf), "Bar<%lld,%s,%s,%lld>",
                thingy, String__cstr(stuff), String__cstr(whatever), blah);
  println(String__from_utf8(buf));
}

} // namespace smoke
} // namespace zefc
