#include "zefc/error.hpp"
#include "zefc/module.hpp"

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
module_load(const char* name)
{
  const auto it = modules().find(name);
  if (it == modules().end() || it->second == nullptr) {
    std::string msg = std::string("could not open ") + name;
    zefc_error(msg.c_str());
  }
  it->second();
}

} // namespace zefc
