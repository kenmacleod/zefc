# Test corpus and stdlib bootstrap

Zef goldens and the mini String/Int/Double/Array runtime used when linking compiler output.

| Path | Role |
|------|------|
| `zef/<case>.zef` | Reference Zef source |
| `expected/<case>.stdout` or `.stderr` | Golden from Zef `.expected` / `.error` |
| `runtime_init.cpp` | `runtime_package_init()` for generated mains |
| `generated/{cstr,int,double,array}.cpp` | Stdlib bootstrap (until folded into `runtime/`) |
| `generated/loadable_modules.cpp` | In-process `module_register` stand-ins for `load(...)` |

**Language coverage:** `meson test --suite compiler` transpiles these `.zef` files with `zefc` and checks goldens. Post-parity follow-ups: [docs/remaining-checklist.md](../../docs/remaining-checklist.md).

**Dispatch ABI** (vtable growth / `ZEFC_SITE`, not a Zef program): `meson test --suite abi` — see [test/abi/](../abi/).

**ScriptBench** (`nbody`, `splay`, `richards`): timed as whole-process compiler executables (load + steady combined). Prefer `/usr/bin/time` or similar when comparing `-Dobject_dispatch=…`.

## Adding a language case

1. Copy `../zef/tests/foo.zef` and `.expected` / `.error` into `zef/` and `expected/`.
2. Wire the case in `compiler/meson.build` (`foreach case_name` or `err_name`, or a runtime-error custom_target like `test39`).
3. Run `meson test -C build --suite compiler`.
