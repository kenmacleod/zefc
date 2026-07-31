# ZefC compiler

Transpiles `.zef` → C++ that uses the shared runtime dispatch ABI (`ZEFC_SEND*`, field IC, `ZEFC_SEL_*` / `ZEFC_SITE`).

**Dispatch A/B is unchanged:** configure Meson with `-Dobject_dispatch=ic|slots|flat|site`. Generated code does not hard-code a send shape.

**Parser lineage:** the lexer/parser/AST here are ZefC-owned (not linked from Zef). When syntax or statement-boundary behavior disagrees with a Zef golden, open the sibling [Zef](https://github.com/pizlonator/zef) tree (`src/parse.cpp` and friends) and port the rule. Linking Zef’s interpreter `Node` types is out of scope; copying grammar/semantics is in scope.

## Status

Compiler suite (`meson test --suite compiler`) covers packages, `load`/`import` scope, inheritance, closures, and more of the smoke goldens.

```bash
ninja -C build compiler/zefc
build/compiler/zefc path/to/file.zef -o out.cpp
# link out.cpp with runtime + stdlib bootstrap (see meson custom_target zefc-hello)
meson test -C build --suite compiler
```
