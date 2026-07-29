// Generated from ../zef/tests/test8.zef (hand-maintained).

#include "zefc/array_api.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test8()
{
  id array = Array__from_ints({1, 2, 3});
  array = Array__push(array, Int__from_i64(42));
  println(array);
  println(Array__at(array, 1));
  Array__mul_assign_at(array, 1, 666);
  println(Array__at(array, 1));
}

} // namespace smoke
} // namespace zefc
