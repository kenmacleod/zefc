# Remaining work checklist

Status after wiring the mirrored Zef smoke corpus (`meson test --suite compiler`: **170/170** green). Language goldens under `test/smoke/` are done; nothing left on the original “wire leftover goldens” list.

## Done (baseline)

- [x] Mirror `zef/tests` into `test/smoke/zef` + `expected/` (stdout / stderr)
- [x] Wire all those goldens in `compiler/meson.build`
- [x] Subclass inheritance / deltablue / fancysubclass / accessible chained assigns

## Semantic fidelity

- [ ] Dynamic parents: replace composition (`zefc_super`) with true shared-layout inheritance
- [ ] Remove or narrow `allow_unresolved` soft-fail for dead-branch names (fancysubclass)
- [ ] Harden `ensure_`: early return must not skip class static field init on re-entrant `Class__new`

## Runtime / stdlib

- [ ] Fold String / Int / Double / Array out of `test/smoke/generated/` into real `runtime/` stdlib
- [ ] Emit typed `ZEFC_IC_SET` / `ZEFC_IC_GET` from codegen where layout is known (runtime helpers exist; setters now yield the RHS for chained assigns)

## Perf / dispatch

See also [dispatch-and-loading.md](dispatch-and-loading.md).

- [ ] Close Fil-C vs g++ gap on ScriptBench (~5–10× today on same dispatch model)
- [ ] Instruction-immediate selectors beyond closed-world `ZEFC_SEL_*` (reloc / text patch under Fil-C W^X)
- [ ] Optional CHA / unguarded sends where safe
- [ ] Build/link-time vtable seal (vs runtime `zefc_vtables_seal()` after last load)

## Repo hygiene

- [ ] Push `main` when ready (local tip may be ahead of `origin/main`)
- [ ] Triage orphan Zef artifact `zef/tests/pprivate3d.zef.expected` (no matching `.zef`; smoke has `private3d`)

## How to extend language coverage

If new Zef tests appear upstream:

1. Copy into `test/smoke/zef/` and `expected/`
2. Wire in `compiler/meson.build`
3. `meson test -C build --suite compiler`
