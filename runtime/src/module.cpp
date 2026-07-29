#include "zefc/error.hpp"
#include "zefc/module.hpp"
#include "zefc/runtime.hpp"

#include <map>
#include <string>

namespace zefc {

namespace {

std::map<std::string, ModuleEntry>&
modules()
{
  static std::map<std::string, ModuleEntry> table;
  return table;
}

} // namespace

void
module_register(const char* name, ModuleEntry entry)
{
  modules()[name] = entry;
}

void
zefc_module_barrier()
{
  selector_sites_patch();
}

void
module_load(const char* name)
{
  const auto it = modules().find(name);
  if (it == modules().end() || it->second == nullptr) {
    std::string msg = std::string("could not open ") + name;
    zefc_error(msg.c_str());
  }
  // Entry may register sites/selectors/methods. Patch after so any sites
  // declared during the entry are filled before the caller continues.
  // Entries that must send during load should call zefc_module_barrier()
  // themselves before those sends.
  it->second();
  zefc_module_barrier();
}

} // namespace zefc
