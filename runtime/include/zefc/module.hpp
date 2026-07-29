#pragma once

namespace zefc {

// Compiled-module load (not Zef source parse). Name matches the path
// string passed to Zef load(), e.g. "stuff/world.zef".
//
// Load sequence: register entry → run entry (may register sites/methods)
// → selector_sites_patch() (intern, grow vtables, patch cells)
// → if the entry only registers, a separate run hook can follow later.
// For smoke, ModuleEntry does define+side effects; we patch *before* the
// entry returns by having entries call zefc_module_barrier() after
// registering sites, OR we patch after entry for modules that only set
// globals without sends during define.
//
// Chosen smoke convention: module_load patches pending sites *after*
// the entry returns if the entry performed no sends; entries that need
// to send must call zefc_module_barrier() first.

using ModuleEntry = void (*)();

void module_register(const char* name, ModuleEntry entry);
void module_load(const char* name);

// Intern pending sites, grow vtables, write selector IDs into cells.
// Safe to call multiple times; clears the pending site list.
void zefc_module_barrier();

} // namespace zefc
