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

std::map<std::string, id>&
package_slots()
{
  static std::map<std::string, id> table;
  return table;
}

std::string
slot_key(const char* pkg, const char* member)
{
  return std::string(pkg) + "\n" + member;
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

bool
package_slot_has(const char* pkg, const char* member)
{
  return package_slots().count(slot_key(pkg, member)) != 0;
}

void
package_slot_set(const char* pkg, const char* member, id value)
{
  package_slots()[slot_key(pkg, member)] = value;
}

id
package_slot_get(const char* pkg, const char* member)
{
  const auto it = package_slots().find(slot_key(pkg, member));
  if (it == package_slots().end()) {
    std::string msg = std::string("cannot resolve get (call with no arguments) named ") + member;
    zefc_error(msg.c_str());
  }
  return it->second;
}

} // namespace zefc
