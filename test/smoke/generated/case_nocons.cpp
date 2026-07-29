// Generated from ../zef/tests/nocons.zef (hand-maintained).

#include "zefc/error.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

void
smoke_nocons()
{
  zefc_error("cannot instantiate class with no constructor");
}

} // namespace smoke
} // namespace zefc
