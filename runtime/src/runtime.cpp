#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/string_api.hpp"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace zefc {

namespace {

std::map<std::string, int> g_selectors;
int g_next_selector = 1;

struct PendingSite {
  int* cell;
  const char* mangled;
};

std::vector<PendingSite>&
pending_sites()
{
  static std::vector<PendingSite> sites;
  return sites;
}

std::vector<VTable*>&
registered_vtables()
{
  static std::vector<VTable*> vts;
  return vts;
}

constexpr int kInitialVTableCapacity = 32;

void
vtable_fill_dnu(VTable* vt, int from, int to)
{
  for (int i = from; i < to; ++i) {
    vt->slots[i] = doesNotUnderstand;
  }
}

} // namespace

id doesNotUnderstand(id self, int selector, ...)
{
  std::fprintf(stderr, "doesNotUnderstand: self=%p selector=%d\n",
               static_cast<void*>(self), selector);
  std::exit(1);
}

int
selector_intern(const char* mangled_name)
{
  const std::string key(mangled_name);
  const auto it = g_selectors.find(key);
  if (it != g_selectors.end()) {
    return it->second;
  }
  const int sel = g_next_selector++;
  g_selectors[key] = sel;
  vtables_ensure_capacity(g_next_selector);
  return sel;
}

int
selector_count()
{
  return g_next_selector;
}

void
selector_site_register(int* cell, const char* mangled_name)
{
  pending_sites().push_back(PendingSite{cell, mangled_name});
}

void
selector_patch(int* slot, int selector)
{
  *slot = selector;
}

void
selector_sites_patch()
{
  auto& sites = pending_sites();
  for (PendingSite& s : sites) {
    const int id = selector_intern(s.mangled);
    *s.cell = id;
  }
  sites.clear();
}

VTable*
vtable_create()
{
  VTable* vt = new VTable();
  vt->capacity = kInitialVTableCapacity;
  vt->slots = new zefc_method[static_cast<size_t>(vt->capacity)];
  vtable_fill_dnu(vt, 0, vt->capacity);
  vtable_register(vt);
  return vt;
}

void
vtable_register(VTable* vt)
{
  registered_vtables().push_back(vt);
  if (selector_count() > vt->capacity) {
    vtables_ensure_capacity(selector_count());
  }
}

void
vtable_set(VTable* vt, int selector, zefc_method method)
{
  if (selector <= 0) {
    std::fprintf(stderr, "invalid selector %d\n", selector);
    ::exit(1);
  }
  if (selector >= vt->capacity) {
    vtables_ensure_capacity(selector + 1);
  }
  vt->slots[selector] = method;
}

void
vtables_ensure_capacity(int min_capacity)
{
  for (VTable* vt : registered_vtables()) {
    if (vt->capacity >= min_capacity) {
      continue;
    }
    int new_cap = vt->capacity;
    while (new_cap < min_capacity) {
      new_cap *= 2;
    }
    zefc_method* neu = new zefc_method[static_cast<size_t>(new_cap)];
    for (int i = 0; i < vt->capacity; ++i) {
      neu[i] = vt->slots[i];
    }
    for (int i = vt->capacity; i < new_cap; ++i) {
      neu[i] = doesNotUnderstand;
    }
    delete[] vt->slots;
    vt->slots = neu;
    vt->capacity = new_cap;
  }
}

void package_register(const char* name)
{
  (void)name;
}

static void
write_value(id value)
{
  const id as_string = ZEFC_SEND0(value, ZEFC_SITE("toString_o"));
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
