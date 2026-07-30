# ZefC compiler

Transpiles `.zef` → C++ that uses the shared runtime dispatch ABI (`ZEFC_SEND*`, field IC, `ZEFC_SEL_*` / `ZEFC_SITE`).

**Dispatch A/B is unchanged:** configure Meson with `-Dobject_dispatch=ic|slots|flat|site`. Generated code does not hard-code a send shape.

## Status

First milestones: `hello.zef`, `test3.zef`, and `accessors.zef` (`my`/`readable`/`accessible`/`static`, field defaults, blocks, multi-arg, dotted get/set).

```bash
ninja -C build compiler/zefc
build/compiler/zefc path/to/file.zef -o out.cpp
# link out.cpp with runtime + stdlib bootstrap (see meson custom_target zefc-hello)
meson test -C build --suite compiler
```
