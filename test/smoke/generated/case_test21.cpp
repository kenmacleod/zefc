// Generated from ../zef/tests/test21.zef (hand-maintained).

#include "zefc/error.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_test21()
{
  zefc_error("parse error at line 2: cannot redeclare function named x");
}

} // namespace smoke
} // namespace zefc
