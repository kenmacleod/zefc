#include "zefc/error.hpp"

#include <cstdio>
#include <cstdlib>

namespace zefc {

void
zefc_error(const char* message)
{
  std::fprintf(stderr, "Error: %s\n", message);
  std::exit(1);
}

} // namespace zefc
