#include "zefc/runtime_bootstrap.hpp"
#include "smoke_cases.hpp"

#include <cstdio>
#include <cstring>

namespace zefc {
namespace smoke {

struct Case {
  const char* name;
  void (*run)();
};

static const Case kCases[] = {
  {"hello", smoke_hello},
  {"test", smoke_test},
  {"precedence", smoke_precedence},
  {"test5", smoke_test5},
  {"test4", smoke_test4},
  {"test3", smoke_test3},
  {"test2", smoke_test2},
  {"super", smoke_super},
  {"staticcall", smoke_staticcall},
  {"nocons", smoke_nocons},
  {"test7b", smoke_test7b},
  {"test6", smoke_test6},
  {"test8", smoke_test8},
  {"test9", smoke_test9},
  {"staticcall2", smoke_staticcall2},
  {"test10", smoke_test10},
};

static void
usage(const char* argv0)
{
  std::fprintf(stderr, "usage: %s <case>\n", argv0);
  std::fprintf(stderr, "cases:");
  for (const Case& c : kCases) {
    std::fprintf(stderr, " %s", c.name);
  }
  std::fprintf(stderr, "\n");
}

} // namespace smoke

} // namespace zefc

int
main(int argc, char** argv)
{
  if (argc != 2) {
    zefc::smoke::usage(argv[0]);
    return 2;
  }

  zefc::runtime_package_init();

  for (const zefc::smoke::Case& c : zefc::smoke::kCases) {
    if (std::strcmp(argv[1], c.name) == 0) {
      c.run();
      return 0;
    }
  }

  zefc::smoke::usage(argv[0]);
  return 2;
}
