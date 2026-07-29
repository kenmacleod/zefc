# Smoke tests

This directory holds the first end-to-end check for ZefC-shaped codegen and runtime behavior.

## Layout

| Path | Role |
|------|------|
| `example.zef` | Zef source input (docs example); future `zefc` compiler input |
| `runtime/cstr.zefc` | `.zefc` input (Orchard/MOC-style + C++); future compiler input |
| `generated/*.cpp` | Expected compiler output (**hand-maintained** until `zefc` exists) |
| `expected/example.stdout` | Golden stdout for `meson test` |
| `main.cpp` | Calls runtime then example module init (deterministic order; production codegen would use `ZEFC_MODULE_CONSTRUCTOR`) |

Shared runtime lives in `../../runtime/` (selectors, dispatch, `println`).

## Build and run

Fil-C++ driver is **`fil++`** (not `filc++`) under the Fil-C install:

`/home/macken01/pizlo/filc-0.678-linux-x86_64/build/bin/fil++`

From the repo root, recommended:

```bash
./tools/setup-build.sh          # meson setup build with CXX=fil++
meson compile -C build
build/test/smoke/zefc-smoke     # prints: hello world
meson test -C build --suite smoke
```

Alternatives:

```bash
CXX=/home/macken01/pizlo/filc-0.678-linux-x86_64/build/bin/fil++ meson setup build
meson setup build --native-file meson/native/filc.ini
```

Use `-Duse_filc=false` and plain `meson setup build` for a system g++ build (e.g. CI without Fil-C).

## When the compiler lands

1. Run `zefc` on `example.zef` and `runtime/cstr.zefc`.
2. Diff (or replace) `generated/*.cpp` with tool output.
3. `meson test -C build --suite smoke` should stay green without changing golden output.
