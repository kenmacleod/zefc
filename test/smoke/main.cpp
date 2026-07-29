#include "zefc/runtime_bootstrap.hpp"
#include "loadable_modules.hpp"
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
  {"package1", smoke_package1},
  {"package2", smoke_package2},
  {"package2b", smoke_package2b},
  {"package2c", smoke_package2c},
  {"package2d", smoke_package2d},
  {"package2e", smoke_package2e},
  {"package2f", smoke_package2f},
  {"package3", smoke_package3},
  {"package4", smoke_package4},
  {"package5", smoke_package5},
  {"package6", smoke_package6},
  {"package7", smoke_package7},
  {"package8", smoke_package8},
  {"package9", smoke_package9},
  {"package10", smoke_package10},
  {"package11", smoke_package11},
  {"package12", smoke_package12},
  {"package12b", smoke_package12b},
  {"package12c", smoke_package12c},
  {"load1", smoke_load1},
  {"load2", smoke_load2},
  {"load3", smoke_load3},
  {"load4", smoke_load4},
  {"load5", smoke_load5},
  {"load6", smoke_load6},
  {"load7", smoke_load7},
  {"load8", smoke_load8},
  {"load9", smoke_load9},
  {"load10", smoke_load10},
  {"hex", smoke_hex},
  {"test20", smoke_test20},
  {"test29", smoke_test29},
  {"test4b", smoke_test4b},
  {"test24", smoke_test24},
  {"test25", smoke_test25},
  {"test25b", smoke_test25b},
  {"test33", smoke_test33},
  {"test34", smoke_test34},
  {"testb", smoke_testb},
  {"teste", smoke_teste},
  {"test37", smoke_test37},
  {"test38", smoke_test38},
  {"test13", smoke_test13},
  {"staticcall3", smoke_staticcall3},
  {"staticcall4", smoke_staticcall4},
  {"staticcall5", smoke_staticcall5},
  {"staticcall6", smoke_staticcall6},
  {"test22", smoke_test22},
  {"super2", smoke_super2},
  {"test16", smoke_test16},
  {"test17", smoke_test17},
  {"test18", smoke_test18},
  {"test11", smoke_test11},
  {"patch1", smoke_patch1},
  {"testc", smoke_testc},
  {"test42", smoke_test42},
  {"test14", smoke_test14},
  {"test15", smoke_test15},
  {"test19", smoke_test19},
  {"test23", smoke_test23},
  {"test26", smoke_test26},
  {"test26b", smoke_test26b},
  {"test26c", smoke_test26c},
  {"test27", smoke_test27},
  {"test30", smoke_test30},
  {"test35", smoke_test35},
  {"test36", smoke_test36},
  {"test43", smoke_test43},
  {"testb2", smoke_testb2},
  {"testb3", smoke_testb3},
  {"testb4", smoke_testb4},
  {"duplicateparam", smoke_duplicateparam},
  {"staticcall7", smoke_staticcall7},
  {"accessors", smoke_accessors},
  {"accessors2b", smoke_accessors2b},
  {"int64", smoke_int64},
  {"private1", smoke_private1},
  {"private2", smoke_private2},
  {"private3", smoke_private3},
  {"classinfunction", smoke_classinfunction},
  {"classinfunction2", smoke_classinfunction2},
  {"test21", smoke_test21},
  {"test28", smoke_test28},
  {"test39", smoke_test39},
  {"accessors2", smoke_accessors2},
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
  zefc::register_smoke_loadable_modules();

  for (const zefc::smoke::Case& c : zefc::smoke::kCases) {
    if (std::strcmp(argv[1], c.name) == 0) {
      c.run();
      return 0;
    }
  }

  zefc::smoke::usage(argv[0]);
  return 2;
}
