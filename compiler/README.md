# ZefC compiler

Transpiles `.zef` → C++ that uses the shared runtime dispatch ABI (`ZEFC_SEND*`, field IC, `ZEFC_SEL_*` / `ZEFC_SITE`).

**Dispatch A/B is unchanged:** configure Meson with `-Dobject_dispatch=ic|slots|flat|site`. Generated code does not hard-code a send shape.

## Status

Milestones so far: `hello`, `test3`, `accessors`, `test30`, `super`, `test4`, `test6`, `test24`/`test25`, `test4b`/`test25b`, `hex`, `precedence`, `super2`.

```bash
ninja -C build compiler/zefc
build/compiler/zefc path/to/file.zef -o out.cpp
# link out.cpp with runtime + stdlib bootstrap (see meson custom_target zefc-hello)
meson test -C build --suite compiler
```
