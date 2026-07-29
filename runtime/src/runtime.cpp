#include "zefc/dispatch.hpp"
#include "zefc/string_api.hpp"

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

namespace zefc {

int zefc_slot_add_o = 0;
int zefc_slot_toString_o = 0;

namespace {

std::map<std::string, int> g_selectors;
int g_next_selector = 1;

} // namespace

id doesNotUnderstand(id self, int selector, ...)
{
  std::fprintf(stderr, "doesNotUnderstand: self=%p selector=%d\n",
               static_cast<void*>(self), selector);
  std::exit(1);
}

int selector_intern(const char* mangled_name)
{
  const std::string key(mangled_name);
  const auto it = g_selectors.find(key);
  if (it != g_selectors.end()) {
    return it->second;
  }
  const int id = g_next_selector++;
  if (id >= kMaxSelectors) {
    std::fprintf(stderr, "selector table full\n");
    std::exit(1);
  }
  g_selectors[key] = id;
  return id;
}

void selector_patch(int* slot, int selector)
{
  *slot = selector;
}

void package_register(const char* name)
{
  (void)name;
}

void println(id value)
{
  if (value == null_id()) {
    std::printf("\n");
    return;
  }
  const id as_string = ZEFC_SEND0(value, zefc_slot_toString_o);
  // String::toString returns self; runtime uses String__cstr from generated code.
  extern const char* String__cstr(id string_obj);
  std::printf("%s\n", String__cstr(as_string));
}

id null_id()
{
  return nullptr;
}

} // namespace zefc
