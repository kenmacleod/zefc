# ABI tests

Runtime/dispatch checks that are **not** Zef source programs.

| Test | What it covers |
|------|----------------|
| `patch1` | `module_load` A then B; B adds a selector (vtable growth); sends via `ZEFC_SITE` |

```bash
meson test -C build --suite abi
```
