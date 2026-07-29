# Smoke tests

End-to-end checks for ZefC-shaped codegen and runtime behavior. Each case mirrors a test from the sibling [Zef](https://github.com/pizlonator/zef) tree (`../zef/tests/`).

**Dispatch ABI:** Today’s hand-compiled cases often use global `zefc_slot_*` loads at each send. That is **scaffolding**, not the target. The intended model is load-time patching of selector immediates so the hot path matches C++ virtual calls; see [docs/dispatch-and-loading.md](../../docs/dispatch-and-loading.md).

## Tier-1 cases (hand-compiled)

| Case | Zef source | What it exercises |
|------|------------|-------------------|
| `hello` | `example.zef` / docs example | String `Foo`, `add`, `println` |
| `test` | `test.zef` | Minimal `println` |
| `precedence` | `precedence.zef` | Int `add` / `sub` / `mul`, operator precedence |
| `test5` | `test5.zef` | `my`, assignment, discarded expression |
| `test4` | `test4.zef` | Nested function / closure call |
| `test3` | `test3.zef` | Class with three fields, `add`, `toString` |
| `test2` | `test2.zef` | `while`, `+=` string build, `print` |
| `super` | `super.zef` | Inheritance, `super`, method override |
| `staticcall` | `staticcall.zef` | Static factory / callable class |
| `nocons` | `nocons.zef` | **Error:** no constructor |
| `test7b` | `test7b.zef` | **Error:** unconstructed superclass fields |
| `test6` | `test6.zef` | Inheritance, `super.doShit`, field `+=` |
| `test8` | `test8.zef` | Array literals, `push`, subscript, `*=` |
| `test9` | `test9.zef` | Nested class + multi-level closure (result 21) |
| `staticcall2` | `staticcall2.zef` | Nested static class call |
| `test10` | `test10.zef` | **Error:** cyclic class hierarchy |
| `package1` | `package1.zef` | Top-level package call |
| `package2` | `package2.zef` | Nested packages |
| `load1` | `load1.zef` | Runtime `module_load` of compiled `stuff/world` |
| `load4` | `load4.zef` | Load package module; read `foo.f` / `foo.x` |
| `load5`–`load10` | `load5`–`load10.zef` | Package overwrite, `import`, scope/import rules |
| `load2`, `load3` | `load2`/`load3.zef` | **Error:** loaded locals not visible to caller |
| `package3`–`package5` | `package3`–`package5.zef` | Merged packages, nested packages, `import` |
| `package2b`–`package2f` | dotted / chained / multi-`import` packages |
| `package6`–`package10` | Classes and package fields / init side effects |
| `package11` | **Error:** unresolved name in package method |
| `package12`–`package12c` | Package identity (`==`) |
| `hex`, `test20`, `test29` | Hex literals; Int `mul`/`add` chaining |
| `test4b`, `test24`, `test25`, `test25b` | Closures / currying |
| `test22` | Property get/set (`fn x` / `set_x`) |
| `test33`, `test34`, `testb`, `teste` | `if`/`while`/`break`; simple fn |
| `test13`, `test37`, `test38` | Multi-arg `String` / `print`/`println` |
| `staticcall3`–`staticcall6` | Nested/package static callables |
| `super2` | Inheritance + `super.foo` I/O |
| `test11`, `test16`–`test18` | **Error:** cyclic class; bare `return`/`break`/`continue` |
| `testc`, `testb2`–`testb4` | Silent/`()` call variants of hello |
| `test14`–`test15`, `test19`, `test35`, `test42`–`test43` | `break`/`continue`/`return`; if-expr |
| `test23`, `test26`–`test27` | Nested closures / HOF call patterns |
| `test30`, `int64`, `duplicateparam` | Identity `==`; 64-bit arith; last duplicate param |
| `test36`, `accessors`, `accessors2b` | Property/`+=`; readable/accessible fields |
| `private1`–`private3`, `classinfunction*` | Private methods; class-in-function |
| `staticcall7` | Deep nested static callable |
| `test21`, `test28`, `test39`, `accessors2` | **Error:** redeclare / bad method / `error()` / private field |

Sources of truth: `expected/<case>.stdout` or `.stderr` (from Zef `.expected` / `.error`). Reference `.zef` files under `zef/`.

## Layout

| Path | Role |
|------|------|
| `zef/*.zef` | Reference inputs (from `../zef/tests/` or docs) |
| `generated/loadable_modules.cpp` | Hand-registered compiled modules for `module_load` |
| `generated/case_*.cpp` | Hand-maintained “compiler output” per case |
| `runtime_init.cpp` | Wires string + int runtime init |
| `main.cpp` | `zefc-smoke <case>` driver |
| `check_stdout.py` | Golden stdout + empty stderr check |
| `check_error.py` | Exit 1 + golden stderr (error cases) |
| `expected/*.stdout` | Expected output |

Shared dispatch/IO lives in `../../runtime/`.

## Build and run

Fil-C++ driver: **`fil++`** at  
`/home/macken01/pizlo/filc-0.678-linux-x86_64/build/bin/fil++`

```bash
./tools/setup-build.sh
meson compile -C build
build/test/smoke/zefc-smoke precedence   # one case
meson test -C build --suite smoke        # all cases (100 today)
```

Use `-Duse_filc=false` and plain `meson setup build` for system g++.

## Adding a case

1. Copy `../zef/tests/foo.zef` and `.expected` into `zef/` and `expected/foo.stdout`.
2. Add `generated/case_foo.cpp` implementing `void zefc::smoke::smoke_foo()` (use an anonymous namespace for helpers to avoid ODR clashes).
3. Declare `smoke_foo()` in `smoke_cases.hpp`, register in `main.cpp` and `meson.build` `smoke_cases` list.

## When the compiler lands

Regenerate `generated/case_*.cpp` from `zef/` inputs; goldens should stay aligned with Zef’s `.expected` files.
