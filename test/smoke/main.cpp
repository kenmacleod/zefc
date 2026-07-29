#include "zefc/runtime.hpp"

namespace zefc {
namespace runtime {
void runtime_package_init();
}
namespace module_example {
void example_module_init();
}
} // namespace zefc

int
main()
{
  zefc::runtime::runtime_package_init();
  zefc::module_example::example_module_init();
  return 0;
}
