# ZefC

> [!NOTE]
> This project was an exploration and not intended for use.
>
> Around 2004 I had an idea on how to use C++-style virtual call tables in a fully dynamic language by recomputing the vtables at module load time, thinking that hash-table lookup was the largest slowdown of dynamic languages at the time.  I was familiar with Objective-C's inline method cache optimization but still thought a vtable would be faster.  A narrow proof-of-concept with a vtable computed at build time was developed in [Orchard-C](https://github.com/kenmacleod/orchard-c) and it wasn't until I discovered [Zef](https://zef-lang.dev/) and with the aid of AI was able to implement a broad test.
>
> I used a transpiler approach, translating Zef language to C++, using my own vtable implementation in place of C++ method calls.  The code successfully implements mosts tests up to the ability to perform 4 of the 5 Zef benchmarks.  I focused on the method call hot path and reviewed that code closely.  (The remaining Zef to C++ translater is not reviewed, except to correct Cursor short-circuiting tests.)  Performance between Zef/JavaScript-style inline-cache and virtual table is tested within this code base using a compile-time flag to avoid external comparisons.
>
> The result of this exploration is that modern (x86) CPUs pipeline and branch prediction perform better with the inline cache (a la JavaScript) than with a vtable (C++ or mine).  Even though the IC hot path is one instruction longer, the CPU traverses the local cache faster than the long distance fetch to the vtable.
>
> My thanks to Filip Pizlo for Zef, Fil-C++, and answering a few brief questions.  Thanks to AI and Cursor in particular for handling the uninteresting details.  With those I was able to close the loop on this 20 year old idea, even if the discovery was to find that not-so modern languages and CPUs had already solved it.

ZefC is a compiler that transpiles the [Zef](https://zef-lang.dev/) language to C++, targeting [Fil-C++](https://github.com/pizlonator/fil-c/) for memory safety and garbage collection. Dispatch uses shared-namespace virtual tables with selector IDs (patterns borrowed from [Orchard-C](https://github.com/kenmacleod/orchard-c)). The build does not link Zef or Orchard; opening those trees to read and port code is welcome (see Lineage).

The end goal is Zef semantics with **C++-like virtual-call cost** on the hot path (~vptr + `vtable[imm]` + call), plus dynamic package loading via load-time selector patching. Vtables alone are not enough for Zef-like ScriptBench speed — see the **performance model** in [docs/dispatch-and-loading.md](docs/dispatch-and-loading.md).

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
3. Golden corpus + stdlib bootstrap under [test/smoke/](test/smoke/) (`zef/`, `expected/`, mini String/Int/Array).
4. Dispatch/load design contract: C++-like hot path, load-time immediate patching ([docs/dispatch-and-loading.md](docs/dispatch-and-loading.md)).
5. **Dispatch ABI** — Growable registry, `VTable` growth, `ZEFC_SITE` / `ZEFC_SEL_*`, NaN-box Double + Int32, field IC + **object_dispatch** (`ic`/`slots`/`flat`/`site`) on `ZEFC_SEND*`. ScriptBench via compiler (`nbody` / `splay` / `richards`); whole-process timing is fine for A/B.
6. **ZefC compiler** — `compiler/zefc` transpiles `.zef` to C++ using the same `ZEFC_*` macros (`meson test --suite compiler`).
7. Broader coverage of the [Zef](https://zef-lang.dev/) test suite via the compiler (packages, and `load` via AOT modules or compile-on-load).
8. Packaging and distribution of the compiler and of code ZefC emits.

## Quick start

```bash
./tools/setup-build.sh          # CXX=fil++ by default; or -Duse_filc=false
meson setup build-gpp -Duse_filc=false -Dobject_dispatch=ic   # or slots|flat|site
meson compile -C build-gpp
meson test -C build-gpp --suite compiler
meson test -C build-gpp --suite abi        # ZEFC_SITE / vtable growth (patch1)
build-gpp/compiler/zefc test/smoke/zef/hello.zef -o /tmp/hello.cpp
```

Generated code links the shared runtime; switch dispatch with `-Dobject_dispatch=` at Meson configure (no compiler flag needed beyond that).

Corpus layout: [test/smoke/README.md](test/smoke/README.md).

## Lineage

| Project | Role |
|--------|------|
| [Zef](https://zef-lang.dev/) ([GitHub](https://github.com/pizlonator/zef)) | Language definition and semantics |
| [Orchard-C](https://github.com/kenmacleod/orchard-c) | Inspiration for vtable / selector dispatch patterns |
| [Fil-C](https://github.com/pizlonator/fil-c/) | Memory-safe C/C++ toolchain (Fil-C++) for ZefC and generated code |

ZefC is a **standalone** build: no compile- or run-time dependency on those repositories. Opening the Zef or Orchard trees to **read and port** grammar, AST decisions, semantics, or tests into this repo is encouraged.

For the front end, prefer a ZefC-owned lexer/parser/AST (suited to C++ codegen) over linking Zef’s interpreter `Node*` graph. Porting rules from Zef’s `parse.cpp` (and related sources) when a golden fails is the intended way to stay aligned — do not invent divergent syntax.
