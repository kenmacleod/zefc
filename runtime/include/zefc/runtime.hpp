#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdlib>

namespace zefc {

using zefc_method = struct id_* (*)(struct id_* self, int selector, ...);

struct id_ {
  zefc_method* isa_;
};

using id = id_*;

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

constexpr int kMaxSelectors = 64;

template<typename T>
T* alloc()
{
  return new T();
}

id doesNotUnderstand(id self, int selector, ...);

int selector_intern(const char* mangled_name);
void selector_patch(int* slot, int selector);

template<typename Fn>
void vtable_set(zefc_method* vtable, int selector, Fn method)
{
  if (selector <= 0 || selector >= kMaxSelectors) {
    std::fprintf(stderr, "invalid selector %d\n", selector);
    ::exit(1);
  }
  vtable[selector] = reinterpret_cast<zefc_method>(method);
}

void package_register(const char* name);
void println(id value);

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
