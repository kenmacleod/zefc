# Smoke tests

End-to-end checks for ZefC-shaped codegen and runtime behavior. Each case mirrors a test from the sibling [Zef](https://github.com/pizlonator/zef) tree (`../zef/tests/`).

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

Sources of truth: `expected/<case>.stdout` or `.stderr` (from Zef `.expected` / `.error`). Reference `.zef` files under `zef/`.

## Layout

| Path | Role |
|------|------|
| `zef/*.zef` | Reference inputs (from `../zef/tests/` or docs) |
| `generated/cstr.cpp`, `int.cpp` | Shared mini-runtime (String, Int) |
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
meson test -C build --suite smoke        # all cases (16 today)
```

Use `-Duse_filc=false` and plain `meson setup build` for system g++.

## Adding a case

1. Copy `../zef/tests/foo.zef` and `.expected` into `zef/` and `expected/foo.stdout`.
2. Add `generated/case_foo.cpp` implementing `void zefc::smoke::smoke_foo()` (use an anonymous namespace for helpers to avoid ODR clashes).
3. Declare `smoke_foo()` in `smoke_cases.hpp`, register in `main.cpp` and `meson.build` `smoke_cases` list.

## When the compiler lands

Regenerate `generated/case_*.cpp` from `zef/` inputs; goldens should stay aligned with Zef’s `.expected` files.
