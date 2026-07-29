// Hand-compiled loadable modules (stand-ins for AOT .zef → .so).

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/module.hpp"
#include "zefc/string_api.hpp"
#include "loadable_modules.hpp"

namespace zefc {

id g_package_foo_f = nullptr;
id g_package_foo_x = nullptr;

namespace {

void
stuff_world()
{
  println(String__from_utf8("world"));
}

void
stuff_package()
{
  // package foo { fn f 42; readable x = 666 }
  g_package_foo_f = Int__from_i64(42);
  g_package_foo_x = Int__from_i64(666);
}

void
stuff_var()
{
  // my x = 42 — local to the loaded module; not visible to the caller.
  (void)Int__from_i64(42);
}

void
stuff_fn()
{
  // fn f 42 — local to the loaded module; not visible to the caller.
  (void)Int__from_i64(42);
}

} // namespace

void
register_smoke_loadable_modules()
{
  module_register("stuff/world.zef", stuff_world);
  module_register("stuff/package.zef", stuff_package);
  module_register("stuff/var.zef", stuff_var);
  module_register("stuff/fn.zef", stuff_fn);
}

} // namespace zefc
