# ZefC

ZefC is a compiler that transpiles the [Zef](https://zef-lang.dev/) language to C++, targeting [Fil-C++](https://github.com/pizlonator/fil-c/) for memory safety and garbage collection. Dispatch uses shared-namespace virtual tables with selector IDs (patterns borrowed from [Orchard-C](https://github.com/kenmacleod/orchard-c); this project does not depend on Zef or Orchard as build dependencies).

The end goal is Zef semantics with **C++-like virtual-call cost** on the hot path (~vptr + `vtable[imm]` + call), plus dynamic package loading via load-time selector patching.

More detail: [docs/index.md](docs/index.md), [docs/dispatch-and-loading.md](docs/dispatch-and-loading.md).

## Features

### From Zef

**Simple Dynamic Types.** Variables and fields do not have static type. All dynamic types are subclasses of `Object`. `int` and `double` are objects. Integers and doubles are both 64-bit. `null` is a synonym for `0`. `false` is a synonym for `0`. `true` is a synonym for `1`.

**Classes.** Zef's class system is most like Ruby's. Fields are private to their instance, except when exposed via accessors.

**Closures.** Classes and functions can be nested to any depth and doing so creates closures. For example, the member functions of a class nested in a function can see the outer function's local variables.

**No Funny Business.** Zef differs from other dynamic languages in that it lacks `eval` and has no monkey patching facilities. The closest thing to `eval` is load, which loads a source file; however the code in that file runs in its own scope. Classes statically declare their fields; new fields cannot be added dynamically. It's not possible to dynamically add fields to objects or to change object types.

**Packages.** Classes and functions can be nested in packages, making them globally visible to anyone who names or imports the package.

**Garbage Collection.** Allocate what you want. The GC cleans it up for you (via Fil-C++).

### Vtable dispatch

**Shared selector namespace.** All method names share a namespace and are assigned a sequential selector id.

**C++-like method dispatch.** `(*(obj->isa_))[selector](obj, selector, ...)` at call sites. Steady-state sends should use a **patched immediate** selector index, matching C++ virtual-call shape—not a per-send global/GOT load for the ID.

**Intermix Zef and C++.** Implementation code in `.zefc` (C++ with ZefC patterns) can mix dispatch with ordinary C++; `.zef` files are Zef-only, transpiled by the ZefC compiler.

**Mangled argument encoding.** Gets, puts, calls, nested-class surfaces, and operators are mangled with their calling arguments to create a selector.

**Short circuits.** Methods that take native types, assert object type or use type coercion, and then use internal accessors to speed operations.

**Undefined methods.** Selectors that are not implemented on a class's hierarchy are filled with `doesNotUnderstand`.

**Dynamic loading.** Packages can load at run time; vtables grow; selector IDs are assigned at load; call-site immediates are patched once (PIC-style). Object/class mutation remains disallowed. TUs compile independently; runtime package load of compiled modules does not require a source parser. See [docs/dispatch-and-loading.md](docs/dispatch-and-loading.md).

### Build

**Meson** + Ninja. Intended to package cleanly under Fedora/Debian guidelines for the C++ toolchain and for language/runtime packages that ship ZefC output.

## Plan so far

1. Language vision and high-level docs ([docs/index.md](docs/index.md)).
2. Meson project, Fil-C++ setup (`tools/setup-build.sh`, native file), minimal shared runtime.
3. Smoke harness: hand-compiled stand-ins for Zef tests, golden stdout/stderr, mini String/Int/Array runtime ([test/smoke/](test/smoke/)).
4. Dispatch/load design contract: C++-like hot path, load-time immediate patching ([docs/dispatch-and-loading.md](docs/dispatch-and-loading.md)).
5. **← We are here** — Implement growable selector registry, vtable growth, and load-time call-site selector patching (replace smoke `zefc_slot_*` scaffolding on the target path). Multi-module / `dlopen`-style load of **compiled** packages; not blocked on parsing Zef source at `load` time.
6. ZefC compiler: parse `.zef` / preprocess `.zefc`, emit C++ that matches the dispatch ABI.
7. Broader coverage of the [Zef](https://zef-lang.dev/) test suite (packages, and `load` via AOT modules or compile-on-load).
8. Packaging and distribution of the compiler and of code ZefC emits.

## Quick start (smoke)

```bash
./tools/setup-build.sh          # CXX=fil++ by default
meson compile -C build
meson test -C build --suite smoke
```

See [test/smoke/README.md](test/smoke/README.md).

## Lineage

| Project | Role |
|---------|------|
| [Zef](https://zef-lang.dev/) ([GitHub](https://github.com/pizlonator/zef)) | Language definition and semantics |
| [Orchard-C](https://github.com/kenmacleod/orchard-c) | Inspiration for vtable / selector dispatch patterns |
| [Fil-C](https://github.com/pizlonator/fil-c/) | Memory-safe C/C++ toolchain (Fil-C++) for ZefC and generated code |

ZefC is a **standalone** project: code and ideas may be borrowed; there is no runtime dependency on those repositories.
