#include "zefc/runtime_bootstrap.hpp"

namespace zefc {
namespace runtime {
void string_runtime_init();
void int_runtime_init();
void double_runtime_init();
void array_runtime_init();
} // namespace runtime

void
runtime_package_init()
{
  runtime::string_runtime_init();
  runtime::int_runtime_init();
  runtime::double_runtime_init();
  runtime::array_runtime_init();
}

} // namespace zefc
