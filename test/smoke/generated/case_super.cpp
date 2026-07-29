// Generated from ../zef/tests/super.zef (hand-maintained).

#include "zefc/io.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

void
Foo_foo()
{
  println(String__from_utf8("hello"));
}

void
Bar_foo()
{
  println(String__from_utf8("world"));
}

void
Bar_bar()
{
  Foo_foo();
}

void
test()
{
  Foo_foo();
  Bar_bar();
  Bar_foo();
}

} // namespace

void
smoke_super()
{
  test();
  test();
}

} // namespace smoke
} // namespace zefc
