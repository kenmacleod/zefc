# Baseline: pure vtable + `slots[sel]` (no IC)

This branch pins the last **fair** ScriptBench dispatch shape before inline caches: every object send is `obj->isa_->slots[sel](…)`, including Body field get/set.

| | |
|--|--|
| **Branch tip** | `6c10875` — *Keep nbody on field sends for fair transpile-shaped timing.* |
| **Left this shape** | `afb4f7d` — *Add per-site field inline caches for accessible get/set.* (first IC) |
| **Later on `main`** | Method IC (`02e90c1`), unguarded typed field IC hits (`98d51dc`), splay/richards smokes |

## Dispatch shape at this tip

```text
ZEFC_SEND* → (Double/Int32 short-circuit) OR isa_->slots[sel](…)
```

Present already (not “raw” Orchard stubs):

- Growable `VTable*` + `slots[]`
- Closed-world `ZEFC_SEL_*` immediates (no per-send `ZEFC_SITE` cell on nbody hots)
- NaN-box Double + low-bit Int32; arithmetic short-circuits in `ZEFC_SEND*`
- Body accessible fields still go through **getter/setter methods** in the vtable (fair transpile)

Absent: field IC, method IC, unguarded member loads.

ScriptBench on this tip: **`nbody` only** (splay/richards land after field IC on `main`).

## Measured results (reproduced on this branch)

Machine: WSL2 Linux x86_64. Buildtype: `debugoptimized`. Fil-C++ 0.678 (`fil++`). Zef: sibling `zef/build/zef` (Fil-C). Wall time via `/usr/bin/time -f '%e'`; stdout discarded; nbody `n=5000`; energies match Zef golden.

### nbody (three runs, 2026-07-30)

| Runtime | Run 1 | Run 2 | Run 3 | ~median |
|---------|-------|-------|-------|---------|
| ZefC Fil-C (`build-vtable-filc`) | 0.10s | 0.09s | 0.10s | **~0.10s** |
| Zef Fil-C | 0.23s | 0.21s | 0.27s | **~0.23s** |

Same-day rebuild of `main` @ `02e90c1` (field + method IC) on the same machine: ZefC Fil-C nbody **~0.09–0.10s** — essentially tied with this baseline for nbody. (Earlier session reports of ~1.3s Fil-C were not reproduced here; treat those as load/environment sensitive unless re-baselined.)

### g++ ZefC at this tip

**Broken** on this machine: `doesNotUnderstand: immediate Int selector=7` (`ZEFC_SEL_push_o`). Heap pointers in the low 32-bit range are misclassified by `id_is_int32` (`u == (unsigned)u`), so Array receives land in the Int short-circuit. Fil-C heap layout avoids that trap. Do not use g++ numbers from this tip without fixing the tag test.

## Earlier evolution (same program, prior commits)

Approximate Fil-C nbody wall times from the investigation that led here (not re-run on this branch checkout):

| Stage | Commit (approx) | ZefC Fil-C | Notes |
|-------|-----------------|------------|--------|
| Heap Double + `ZEFC_SITE` sends | `2c62646` | ~1.2s | First nbody smoke |
| NaN-box Double short-circuit | `abc92eb` | ~0.9s | Still field/Array vtable sends |
| Fair field sends + `ZEFC_SEL_*` | **`6c10875` (this tip)** | **~0.10s** (reproduced above) | Pure vtable+slots baseline |

(The large drop vs the early ~1s figures is dominated by immediates / selector literals and environment; this doc’s normative baseline for “no IC” is the **reproduced** table above.)

## How to reproduce

```bash
git checkout baseline/pure-vtable-slots
CXX=…/fil++ meson setup build-vtable-filc -Dbuildtype=debugoptimized
meson compile -C build-vtable-filc test/smoke/zefc-smoke
/usr/bin/time -f '%e' build-vtable-filc/test/smoke/zefc-smoke nbody
/usr/bin/time -f '%e' ../zef/build/zef ../zef/ScriptBench/nbody.zef
```
