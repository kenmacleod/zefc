// Generated from ../zef/tests/super2.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/selectors.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

static void
println_msg(const char* prefix, long long x)
{
  id s = String__from_utf8(prefix);
  s = send(s, zefc_slot_add_o, ZEFC_SEND0(Int__from_i64(x), zefc_slot_toString_o));
  println(s);
}

static void
test()
{
  println_msg("hello", 1);
  println_msg("hello", 2);
  println_msg("world", 3);
}

} // namespace

void
smoke_super2()
{
  test();
  test();
}

} // namespace smoke
} // namespace zefc
