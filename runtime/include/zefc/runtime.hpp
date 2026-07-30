#pragma once

#include "zefc/object_dispatch.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace zefc {

using zefc_method = struct id_* (*)(struct id_* self, int selector, ...);

// Class dispatch table. slots[] may grow; VTable* identity is stable.
struct VTable {
  zefc_method* slots;
  int capacity;
};

#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT
// Plan A: object points at the method array (Orchard-style isa_[sel]).
using IsaPtr = zefc_method*;
#else
// Handle: object points at VTable; send uses isa_->slots[sel].
using IsaPtr = VTable*;
#endif

struct id_ {
  IsaPtr isa_;
};

using id = id_*;

// Zef-style immediate doubles: IEEE bits + magic stored in the id pointer bits.
// Immediate int32: low bit set, payload in bits 1..32 (pointers are even-aligned).
// Heap object pointers are even and below the double tag. Never deref an immediate.
inline constexpr uintptr_t kDoubleTagMagic = 0x1000000000000ull;

inline bool
id_is_double(id v)
{
  return reinterpret_cast<uintptr_t>(v) >= kDoubleTagMagic;
}

inline bool
id_is_int32(id v)
{
  const uintptr_t u = reinterpret_cast<uintptr_t>(v);
  return u < kDoubleTagMagic && (u & 1u) != 0;
}

inline bool
id_is_object(id v)
{
  const uintptr_t u = reinterpret_cast<uintptr_t>(v);
  return u != 0 && u < kDoubleTagMagic && (u & 1u) == 0;
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
void selector_reserve(const char* mangled_name, int id);
int selector_count(); // highest assigned ID + 1 (0 unused)

// --- Call-site patch cells (selector IDs) ---
void selector_site_register(int* cell, const char* mangled_name);
void selector_sites_patch();
void selector_patch(int* slot, int selector);

// --- Growable vtables ---
VTable* vtable_create();
void vtable_register(VTable* vt);
void vtable_set(VTable* vt, int selector, zefc_method method);
void vtables_ensure_capacity(int min_capacity);

template<typename Fn>
void vtable_set(VTable* vt, int selector, Fn method)
{
  vtable_set(vt, selector, reinterpret_cast<zefc_method>(method));
}

// Map object isa_ ↔ VTable* / method slot (layout-independent helpers).
VTable* zefc_vtable_of(id obj);
zefc_method zefc_method_at(id obj, int selector);
void zefc_set_isa(id obj, VTable* vt);

template<typename T>
inline void
zefc_set_isa(T* obj, VTable* vt)
{
  zefc_set_isa(as_id(obj), vt);
}

#if ZEFC_OBJECT_DISPATCH == ZEFC_OD_FLAT
// Live objects whose isa_ must be rewritten when vt->slots relocates.
void zefc_object_register(id obj, VTable* vt);
#endif

void package_register(const char* name);

id null_id();

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
