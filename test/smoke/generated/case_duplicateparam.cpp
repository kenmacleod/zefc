// Generated from ../zef/tests/duplicateparam.zef (hand-maintained).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_duplicateparam()
{
  println(Int__from_i64(3));
  println(Int__from_i64(3));
}

} // namespace smoke
} // namespace zefc
