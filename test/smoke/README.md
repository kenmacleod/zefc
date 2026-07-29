# Smoke tests

End-to-end checks against Zef goldens from the sibling [Zef](https://github.com/pizlonator/zef) tree (`../zef/tests/`). Each case has:

| Path | Role |
|------|------|
| `zef/<case>.zef` | Reference Zef source (what the language does) |
| `generated/case_<case>.cpp` | Hand-maintained stand-in for compiler output |
| `expected/<case>.stdout` or `.stderr` | Golden from Zef `.expected` / `.error` |

**`zef/` ↔ `generated/` fidelity varies** — see [Fidelity](#fidelity). Prefer **structure** cases when studying dispatch; many package prints are still stubs. The comment `Generated from … (hand-maintained)` means paired goldens, not that every file is a full lowering.

**Dispatch ABI:** Known names use closed-world `ZEFC_SEL_*` integer literals (`vtable[imm]`); late/dynamic names use `ZEFC_SITE("…")` cells ([docs/dispatch-and-loading.md](../../docs/dispatch-and-loading.md)). Ideal end state: reloc/text-imm patch for the dynamic set too.

## Fidelity

Three kinds of hand-compiles. When matching `.zef` to `.cpp`, start from this list — not from the case name.

### Structure

C++ has objects / closures / vtables / `send` you can map to the Zef (transpile-shaped for dispatch prove-out).

| Cases | Notes |
|-------|--------|
| `hello` | Class `Foo`, `add`, `println` via sends |
| `test3` | Multi-field class, `add`, `toString` |
| `test4`, `test4b`, `test24` | Nested fn / capturing closure (call via `add_o` slot) |
| `test25`, `test25b`, `test23` | Curried / multi-capture closures |
| `test9` | `Foo.stuff` → inner closure → `Bar.thingy` |
| `test6` | `Foo`/`Bar` inheritance, `super.doShit`, field `+=`, `toString` |
| `super`, `super2` | Override + `super.foo` as superclass method send |
| `precedence`, `test20`, `test29` | Int `add` / `sub` / `mul` through dispatch |
| `test2` | `while` + string `add` / `print` |
| `test8` | Array mini-runtime (`push`, subscript, `*=`) |
| `test13`, `test37`, `test38` | Multi-part string built with `String`/`Int` sends |
| `package6`, `package7` | Class instance with field printed via `toString` |
| `test22`, `test36` | Property get/set (`x` / `set_x`) via sends |
| `accessors`, `accessors2b` | Readable/accessible (+ private via same-class methods) |
| `private1`–`private3` | Public → private method / static create chains |
| `classinfunction`, `classinfunction2` | Fn returns class; ctor captures outer; `baz` get |
| `staticcall`–`staticcall7` | Callable class / nested static `call` chains |
| `test26`, `test26b`, `test26c`, `test27` | HOF `times`/`foo` invoking closure objects via `call` |
| `test30` | Instance identity `==` via `eq` send |
| `patch1` | **ABI acceptance:** load A then B; site cells; vtable growth |
| `nbody` | ScriptBench n-body: monomorphic Body field loads/stores; Double imm + `ZEFC_SEL_*` |

### Behavioral / sequencing

Control flow, load order, or print sequence matches the Zef; little or no method dispatch on user types.

| Cases | Notes |
|-------|--------|
| `test`, `test5`, `testc`, `testb`, `testb2`–`testb4`, `teste` | Simple print / locals / loops |
| `hex`, `int64` | Literals and arith in C++; results boxed for print |
| `test14`, `test15`, `test19`, `test33`, `test34`, `test35`, `test42`, `test43` | `if` / `while` / `break` / `continue` / `return` as C++ |
| `package3`, `package4` | Package merge / import **outputs** via string `add` |
| `load1`–`load10` | In-process `module_load` + package bindings / scope rules |

### Golden stub

Hardcodes the golden result (or empty body). Do **not** expect the C++ to mirror the `.zef` structure.

| Cases | Typical shape |
|-------|----------------|
| `package1`, `package2`, `package2b`–`package2f`, `package5`, `package8`–`package10` | Print `"hello"` or empty |
| `package12`, `package12b`, `package12c` | Print identity `0`/`1` literals |
| `duplicateparam` | Print literal ints |

### Error stub

Calls `zefc_error("…")` with Zef’s message text; no real rejection logic.

`nocons`, `test7b`, `test10`, `test11`, `test16`–`test18`, `test21`, `test28`, `test39`, `accessors2`, `package11`, `load2`, `load3`

(`load2` / `load3` still run `module_load` first, then error.)

## Layout

| Path | Role |
|------|------|
| `zef/*.zef` | Reference inputs |
| `generated/loadable_modules.cpp` | Hand-registered modules for `module_load` |
| `generated/case_*.cpp` | Hand-maintained stand-ins (see Fidelity) |
| `runtime_init.cpp` | String + int (+ array) runtime init |
| `main.cpp` | `zefc-smoke <case>` driver |
| `check_stdout.py` / `check_error.py` | Golden checks |
| `expected/*` | Expected stdout / stderr |

Shared dispatch/IO: `../../runtime/`.

## Build and run

Fil-C++ driver: **`fil++`** at  
`/home/macken01/pizlo/filc-0.678-linux-x86_64/build/bin/fil++`

```bash
./tools/setup-build.sh
meson compile -C build
build/test/smoke/zefc-smoke precedence   # one case
meson test -C build --suite smoke        # all cases (101 today)
```

Use `-Duse_filc=false` and plain `meson setup build` for system g++.

## Adding a case

1. Copy `../zef/tests/foo.zef` and `.expected` / `.error` into `zef/` and `expected/`.
2. Add `generated/case_foo.cpp` implementing `smoke_foo()` (anonymous namespace for helpers).
3. Prefer **structure** when the case is meant to exercise dispatch; use **stub** only when expanding golden coverage cheaply — and list it under the right fidelity section in this README.
4. Declare in `smoke_cases.hpp`, register in `main.cpp` and `meson.build`.

## When the compiler lands

Regenerate `generated/case_*.cpp` from `zef/` inputs; goldens stay aligned with Zef’s `.expected` / `.error`. Fidelity tiers above go away as real lowering replaces stubs.
