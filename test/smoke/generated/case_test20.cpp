// Generated from ../zef/tests/test20.zef (hand-maintained).

#include "zefc/dispatch.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/selectors.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test20()
{
  id v = Int__from_i64(42);
  v = send(v, zefc_slot_mul_o, Int__from_i64(666));
  v = send(v, zefc_slot_add_o, Int__from_i64(1410));
  println(v);
}

} // namespace smoke
} // namespace zefc
