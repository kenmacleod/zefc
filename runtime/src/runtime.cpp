#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/string_api.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

namespace zefc {

int zefc_slot_add_o = 0;
int zefc_slot_sub_o = 0;
int zefc_slot_mul_o = 0;
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
  const int sel = g_next_selector++;
  if (sel >= kMaxSelectors) {
    std::fprintf(stderr, "selector table full\n");
    std::exit(1);
  }
  g_selectors[key] = sel;
  return sel;
}

void selector_patch(int* slot, int selector)
{
  *slot = selector;
}

void package_register(const char* name)
{
  (void)name;
}

static void
write_value(id value)
{
  const id as_string = ZEFC_SEND0(value, zefc_slot_toString_o);
  std::fputs(String__cstr(as_string), stdout);
}

void print(id value)
{
  write_value(value);
  std::fflush(stdout);
}

void println(id value)
{
  if (value == null_id()) {
    std::printf("\n");
    return;
  }
  write_value(value);
  std::printf("\n");
}

id null_id()
{
  return nullptr;
}

} // namespace zefc
