#include "zefc/dispatch.hpp"
#include "zefc/io.hpp"
#include "zefc/known_selectors.hpp"
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

#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT
std::map<id, VTable*>&
live_objects()
{
  static std::map<id, VTable*> objs;
  return objs;
}

std::map<zefc_method*, VTable*>&
slots_to_vtable()
{
  static std::map<zefc_method*, VTable*> map;
  return map;
}
#endif

constexpr int kInitialVTableCapacity = 32;

bool g_vtables_sealed = false;

void
vtable_fill_dnu(VTable* vt, int from, int to)
{
  for (int i = from; i < to; ++i) {
    vt->slots[i] = doesNotUnderstand;
  }
}

[[noreturn]] void
sealed_grow_abort(const char* what)
{
  std::fprintf(stderr, "zefc: %s after zefc_vtables_seal()\n", what);
  std::exit(1);
}

} // namespace

void
zefc_vtables_seal()
{
  g_vtables_sealed = true;
#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT
  live_objects().clear();
#endif
}

bool
zefc_vtables_sealed()
{
  return g_vtables_sealed;
}

VTable*
zefc_vtable_of(id obj)
{
  if (!id_is_object(obj)) {
    return nullptr;
  }
#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT
  const auto it = slots_to_vtable().find(obj->isa_);
  if (it == slots_to_vtable().end()) {
    std::fprintf(stderr, "zefc_vtable_of: unknown isa_ %p\n", static_cast<void*>(obj->isa_));
    std::exit(1);
  }
  return it->second;
#else
  return obj->isa_;
#endif
}

zefc_method
zefc_method_at(id obj, int selector)
{
#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT
  return obj->isa_[selector];
#else
  return obj->isa_->slots[selector];
#endif
}

void
zefc_set_isa(id obj, VTable* vt)
{
  if (!obj || !vt) {
    std::fprintf(stderr, "zefc_set_isa: null\n");
    std::exit(1);
  }
#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT
  obj->isa_ = vt->slots;
  // Class slots→VTable map is maintained only at vtable_create / grow.
  // Before seal, track instances so grow can rewrite isa_; after seal, just
  // assign the stable slots pointer (same cost shape as slots-mode set_isa).
  if (!g_vtables_sealed) {
    live_objects()[obj] = vt;
  }
#else
  obj->isa_ = vt;
#endif
}

id doesNotUnderstand(id self, int selector, ...)
{
  const char* name = "?";
  for (const auto& entry : g_selectors) {
    if (entry.second == selector) {
      name = entry.first.c_str();
      break;
    }
  }
  std::fprintf(stderr, "doesNotUnderstand: self=%p selector=%d (%s)\n",
               static_cast<void*>(self), selector, name);
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
  if (g_vtables_sealed) {
    sealed_grow_abort("new selector_intern");
  }
  const int sel = g_next_selector++;
  g_selectors[key] = sel;
  vtables_ensure_capacity(g_next_selector);
  return sel;
}

void
selector_reserve(const char* mangled_name, int id)
{
  if (id <= 0) {
    std::fprintf(stderr, "selector_reserve: invalid id %d for %s\n", id, mangled_name);
    std::exit(1);
  }
  const std::string key(mangled_name);
  const auto it = g_selectors.find(key);
  if (it != g_selectors.end()) {
    if (it->second != id) {
      std::fprintf(stderr, "selector_reserve: %s already id %d, wanted %d\n",
                   mangled_name, it->second, id);
      std::exit(1);
    }
    return;
  }
  for (const auto& entry : g_selectors) {
    if (entry.second == id) {
      std::fprintf(stderr, "selector_reserve: id %d already used by %s (reserving %s)\n",
                   id, entry.first.c_str(), mangled_name);
      std::exit(1);
    }
  }
  if (g_vtables_sealed) {
    sealed_grow_abort("selector_reserve of new name");
  }
  g_selectors[key] = id;
  if (id + 1 > g_next_selector) {
    g_next_selector = id + 1;
  }
  vtables_ensure_capacity(g_next_selector);
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
#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT
  slots_to_vtable()[vt->slots] = vt;
#endif
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
    if (g_vtables_sealed) {
      sealed_grow_abort("vtables_ensure_capacity grow");
    }
    int new_cap = vt->capacity;
    while (new_cap < min_capacity) {
      new_cap *= 2;
    }
    zefc_method* old_slots = vt->slots;
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
#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT
    slots_to_vtable().erase(old_slots);
    slots_to_vtable()[neu] = vt;
    for (auto& entry : live_objects()) {
      if (entry.second == vt && entry.first) {
        entry.first->isa_ = neu;
      }
    }
#else
    (void)old_slots;
#endif
  }
}

void package_register(const char* name)
{
  (void)name;
}

static void
write_value(id value)
{
  if (!value) {
    // Zef prints null as 0 (same as int zero / falsy).
    std::fputs("0", stdout);
    return;
  }
  const id as_string = ZEFC_SEND0(value, ZEFC_SEL_toString_o);
  std::fputs(String__cstr(as_string), stdout);
}

void print(id value)
{
  write_value(value);
  std::fflush(stdout);
}

void println(id value)
{
  write_value(value);
  std::printf("\n");
}

id null_id()
{
  return nullptr;
}

} // namespace zefc
