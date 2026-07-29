#pragma once

namespace zefc {

// Compiled-module load (not Zef source parse). Name matches the path
// string passed to Zef load(), e.g. "stuff/world.zef".
using ModuleEntry = void (*)();

void module_register(const char* name, ModuleEntry entry);
void module_load(const char* name);

} // namespace zefc
