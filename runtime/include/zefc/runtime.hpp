#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace zefc {

using zefc_method = struct id_* (*)(struct id_* self, int selector, ...);

// Stable vtable handle: slots[] may grow; the VTable* identity does not move.
struct VTable {
  zefc_method* slots;
  int capacity;
};

struct id_ {
  VTable* isa_;
};

using id = id_*;

// Zef-style immediate doubles: IEEE bits + magic stored in the id pointer bits.
// Heap object pointers stay below the tag (Fil-C heap verified). Never deref an immediate.
inline constexpr uintptr_t kDoubleTagMagic = 0x1000000000000ull;

inline bool
id_is_double(id v)
{
  return reinterpret_cast<uintptr_t>(v) >= kDoubleTagMagic;
}

inline bool
id_is_object(id v)
{
  return v != nullptr && !id_is_double(v);
}

// Heap objects use the same header layout as id_ (isa_ first).
template<typename Body>
Body*
body(id obj)
{
  return reinterpret_cast<Body*>(obj);
}

template<typename Body>
id as_id(Body* obj)
{
  return reinterpret_cast<id>(obj);
}

template<typename T>
T* alloc()
{
  return new T();
}

id doesNotUnderstand(id self, int selector, ...);

// --- Selector registry (append-only) ---
int selector_intern(const char* mangled_name);
// Bind mangled_name to a fixed ID (closed-world immediates). Call before conflicting interns.
void selector_reserve(const char* mangled_name, int id);
int selector_count(); // highest assigned ID + 1 (0 unused)

// --- Call-site patch cells ---
// Register a site cell to receive the interned ID for mangled_name at next patch.
void selector_site_register(int* cell, const char* mangled_name);
// Intern all pending site names, grow vtables, write IDs into cells, clear pending.
void selector_sites_patch();

// Compat: write a single cell (also used during gradual migration).
void selector_patch(int* slot, int selector);

// --- Growable vtables ---
VTable* vtable_create();
void vtable_register(VTable* vt); // participate in growth
void vtable_set(VTable* vt, int selector, zefc_method method);
void vtables_ensure_capacity(int min_capacity);

template<typename Fn>
void vtable_set(VTable* vt, int selector, Fn method)
{
  vtable_set(vt, selector, reinterpret_cast<zefc_method>(method));
}

void package_register(const char* name);

id null_id();

// Lower priority number runs earlier (101 before 102).
#define ZEFC_MODULE_CONSTRUCTOR(priority, fn) \
  static void fn(); \
  __attribute__((constructor(priority))) static void ZEFC_ANONYMIZE(zefc_ctor_, __LINE__)(void) \
  { \
    fn(); \
  } \
  static void fn()

#define ZEFC_ANONYMIZE(a, b) ZEFC_ANONYMIZE_IMPL(a, b)
#define ZEFC_ANONYMIZE_IMPL(a, b) a##b

} // namespace zefc
