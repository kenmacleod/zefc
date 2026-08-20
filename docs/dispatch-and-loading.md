# Dispatch and dynamic loading

This document is the contract for ZefC’s Orchard-style dispatch under dynamic package loading. Language overview lives in [index.md](index.md).

## Goals

- **Match C++ virtual-call cost on the hot path.** On x86-64 that is roughly three operations: load vptr, load `vtable[imm]`, `call`.
- **Allow packages to load after other TUs exist** without rebuilding every translation unit or assigning selector IDs in a closed world at compile time.
- **Preserve Zef’s “no funny business”** for objects/classes: load-time patching affects dispatch tables and call-site constants, not arbitrary object/class mutation.

## Performance model (where we are vs headed)

**Lesson:** Orchard vtables alone are **not** Zef parity. Zef’s ScriptBench wins come from **several** hot-path shapes working together under Fil-C++, not from “virtual calls only.”

| Mechanism | Role | ZefC today | Headed |
|-----------|------|------------|--------|
| **Vtable send** | Polymorphic / unknown methods | Default **method IC**; A/B via `-Dobject_dispatch=ic\|slots\|flat\|site` (`ZEFC_OBJECT_DISPATCH`): guarded IC, `isa_->slots[sel]`, flat `isa_[sel]` + instance fix-up on grow, or sticky site callee | Optional CHA unguarded; text/reloc sel/callee |
| **Selector immediates** | Keep `sel` out of a per-send global/GOT load | **Closed-world** `ZEFC_SEL_*` literals; dynamic names use `ZEFC_SITE` cells | Reloc / text-imm patch for late names under Fil-C W^X |
| **Immediate values** | Int/Double ops without heap + vtable | NaN-box Double; low-bit Int32; `ZEFC_SEND*` short-circuits | Keep; extend bitops/cmp short-circuits as benches need |
| **Field IC** | Accessible get/set is most of nbody/splay traffic | `ZEFC_IC_GET/SET(obj, sel, Type, member)`: miss primes site; **steady-state unguarded typed member load/store** (Zef UnguardedAccessCache-style). `ZEFC_IC_*_OFFSET` for polymorphic sites | Keep |
| **Fil-C++** | Memory safety + GC for runtime and generated code | Both Zef and ZefC Fil-C builds are the fair comparison; g++ is a separate baseline | Same. Fil-C allows in-bounds field loads; it does **not** make an extra indirect call free |

**Fairness:** optimize shared runtime and transpile-shaped emission (above). Do not hand-specialize one bench with raw `body->field` unless CHA/compiler would. Typed field IC is the general emit shape when the site’s class layout is known. `send(recv, sel, arg)` with a *varying* selector always uses the table helper (`zefc_method_at`).

**Object dispatch (perf switch):** Meson `-Dobject_dispatch=ic|slots|flat|site` sets `ZEFC_OBJECT_DISPATCH` (see `object_dispatch.hpp` / `dispatch.hpp`). Call sites stay `ZEFC_SEND*`; only the object path changes. Immediates and field IC unchanged. Use separate build dirs at the same `buildtype=debugoptimized` when comparing.

| Value | Hot path | Notes |
|-------|----------|--------|
| `ic` (default) | guard `isa_` + cached callee | Zef-style method IC |
| `slots` | `isa_->slots[sel]` | `VTable*` handle (no IC) |
| `flat` | `isa_[sel]` | Plan A: `IsaPtr = zefc_method*`; grow rewrites live `isa_` until `zefc_vtables_seal()` |
| `site` | sticky `CallSite.callee` | Plan B: fill on first use (optional `call_site_bind` at load); **no isa_ guard** — wrong under polymorphism (e.g. richards). Prefer `ic`/`slots`/`flat` for full compiler suite |

### ScriptBench wall times (matched `-O2`)

Machine: WSL2 x86_64. nbody/richards/splay: 2026-07-30; deltablue: 2026-07-31. All builds `debugoptimized` (`optimization=2`). Fil-C++ 0.678; ZefC g++ = system g++. Five warm runs; approximate medians. Field IC on in all ZefC columns. Zef has no g++ build.

**What is timed:** whole-process wall of the compiler-built `nbody` / `splay` / `richards` executables (init + steady combined). That is enough for dispatch A/B; we no longer split an AFTER STARTUP phase in the harness.

**Future optimization:** seal (closed-world freeze) can move to **build/link time** so the process starts already sealed; today’s runtime `zefc_vtables_seal()` after last load is the stand-in when used.

**Fil-C** (`-Dobject_dispatch=…`; full-process walls):

| Bench | ic | slots | flat | site | Zef |
|-------|----|-------|------|------|-----|
| nbody | ~0.05s | ~0.06s | ~0.06s | ~0.10s | ~0.21s |
| richards | ~0.04s | ~0.04s | ~0.03s | *(crash — poly)* | ~0.19s |
| splay | ~1.4s | ~0.94s | ~1.0s | ~1.4s | ~2.6s |
| deltablue | ~0.17s | ~0.15s | ~0.19s | *(fail — poly)* | ~0.48s |

**g++** (same ZefC code; unsafe / no Fil-C checks):

| Bench | ic | slots | flat | site |
|-------|----|-------|------|------|
| nbody | ~0.00s | ~0.01s | ~0.01s | ~0.01s |
| richards | ~0.00s | ~0.00s | ~0.00s | *(crash — poly)* |
| splay | ~0.25s | ~0.16s | ~0.16s | ~0.22s |
| deltablue | ~0.04s | ~0.03s | ~0.04s | *(crash — poly)* |

**Takeaways:** `ic` vs `slots` vs `site` (where correct) stay in the noise on these benches. Unsealed `flat` paid a live-object map on every `zefc_set_isa` (splay ~3.9s Fil-C / ~0.67s g++). After `zefc_vtables_seal()`, flat init is just `isa_ = vt->slots` (class `slots→VTable*` map is create/grow only). Expect flat ≈ slots on post-seal walls; splay is field-IC / alloc dominated so the one-load send win stays noise. `site` is invalid under polymorphism (`richards`). Fil-C vs g++ remains ~5–10× on the same dispatch model. ZefC Fil-C vs Zef is still mostly AOT vs interpret.

**Seal:** `zefc_vtables_seal()` marks the process closed-world: no new selectors / vtable grows; flat drops `live_objects` and never re-registers instances. Call after the last package load / last selector intern — analogous to Obj-C after dyld settles.

ScriptBench cases (`nbody`, `splay`, `richards`, `deltablue`) run via `--suite compiler` (same workload as `zef/ScriptBench`); see [test/smoke/README.md](../test/smoke/README.md).

## Non-goals (hot path)

- Loading a selector ID from a global, GOT, or registry **on every send**.
- String or hash lookup of method names on every send.
- Rewriting call sites continuously after they have been patched and user code is running.

Global `zefc_slot_*` variables in early scaffolding are obsolete; prefer `ZEFC_SEL_*` / `ZEFC_SITE`. New work must move toward the ABI below.

## Independent translation units (not blocked)

In Zef, `load("path.zef")` is a **runtime** builtin. Parsing the *caller* only builds a call with a string argument; the callee file is opened, parsed, resolved, and evaluated when that call runs. Compiling or parsing the current file does **not** require parsing the loaded file.

ZefC therefore compiles **each TU independently**. Dynamic loading is only a **runtime** concern (map a compiled module, register selectors, grow vtables, patch call-site immediates, run init). Registry / patching / multi-module checks are **not** blocked on a ZefC source parser or on compile-on-load.

## Two different “load” stories

| Mechanism | Meaning | Blocks next milestone? |
|-----------|---------|------------------------|
| **ZefC package load** | Map a **compiled** module (e.g. shared object or registered image), register selectors, grow vtables, patch call sites, then run module init / top-level. | **No** — in-process `module_register` / `module_load` and compiler `load` tests cover sequencing. |
| **Zef `load("….zef")` as source** | Interpreter: parse and evaluate source at the call. | **No** for dispatch work. Later: either AOT (`load` resolves to a prebuilt module on a search path) or compile-on-load. |

In-process registration of loadable modules (`test/smoke/generated/loadable_modules.cpp`) plus `--suite abi` (`patch1`) prove load sequencing and vtable growth. Compiler `load*.zef` goldens cover language-facing `load` / `import`.

## Call-site ABI (steady state)

Sends keep the Orchard shape:

```text
(*(obj->isa_))[selector](obj, selector, …)
```

After load-time patching, **`selector` is a compile-/load-time constant** usable as an immediate (or equivalent displacement) in the addressing mode for the vtable load. Passing `selector` as an argument is part of the Orchard calling convention and does not imply an extra memory load of the ID if the ID is already an immediate in a register or insn.

**Target:** no per-send memory load of the selector ID beyond what C++ already does for a fixed vtable slot index.

## Load-time sequence

For each newly loaded module:

1. **Map** the module (`dlopen` or equivalent in-process registration for early milestones).
2. **Register selectors** — mangled names enter a shared, append-only namespace; each new name gets the next ID.
3. **Grow vtables** — every live class vtable expands to the new width; new slots are filled with `doesNotUnderstand`; the loading package installs its overrides.
4. **Patch call sites** — rewrite immediates / reloc records so each send that names a selector now embeds that ID. Analogous to PIC relocation: **once**, then freeze.
5. **Run** module constructors / package top-level / entry points.

User code from that module must not run until step 4 completes for that module’s patch set.

## Selector registry

- All method names (including mangled get/put/call/operator forms) share **one** selector namespace process-wide.
- IDs are **append-only**. Existing IDs are never renumbered.
- The first registration of a mangled name assigns its ID; later loads reuse it.
- Closed-world / fully linked builds may bake immediates at compile time and omit a patch table for selectors known then; dynamic loads still use the registry for anything new.

## Patching mechanism

**Chosen representation (this milestone):** per-call-site **addressable patch cells** — `static int` cells registered via `ZEFC_SITE("mangled")` / `selector_site_register`, written once by `selector_sites_patch()` / `zefc_module_barrier()` (also at the end of `module_load`). Sends use the cell value as the vtable index:

```text
obj->isa_->slots[site](obj, site, …)
```

where `isa_` is a stable `VTable*` (slots array may grow; the handle does not move).

This is the portable step toward instruction-immediate selectors. It removes shared per-send `zefc_slot_*` loads at new sites (each site has its own cell). **Not yet** true `vtable[imm]` in the instruction stream for arbitrary dynamic names; that remains a follow-on (reloc / text patch under Fil-C W^X).

**Closed-world immediates (landed):** well-known mangled names are reserved at process start via `selector_reserve` / `known_selectors_init()` into fixed `ZEFC_SEL_*` enum IDs (`known_selectors.hpp`). Generated (or ABI-test) code may pass those **integer literals** into `ZEFC_SEND*`, so the compiler can emit `vtable[imm]` with no cell load. `ZEFC_SITE` remains for late/dynamic names (`test/abi/patch1`, open-ended modules).

Compat: shared `zefc_slot_*` globals have been **removed**. Sends use either `ZEFC_SEL_*` literals or `ZEFC_SITE` (lazy-intern into a per-site static cell).

**Already-loaded code that later needs a brand-new selector:** only sites that **reference** that selector need patching. Append-only IDs keep old immediates/cells valid. Vtables grow; new slots are `doesNotUnderstand` until the loading module installs methods.

## Vtable growth

When the global selector count increases:

- Each registered `VTable` reallocates its `slots` array (capacity doubles as needed).
- New entries are filled with `doesNotUnderstand`.
- Existing entries are preserved.
- Objects hold a stable `VTable*`; they do not need per-object updates when slots grow.

## Module layout (first milestone)

Minimum viable package unit:

- In-process `module_register` / `module_load` (no `dlopen` yet).
- `module_load`: run entry → `zefc_module_barrier()` (intern pending sites, grow vtables, patch cells).
- Entries that must send during load call `zefc_module_barrier()` before those sends.

## Scaffolding vs end state

| Today | Target |
|---------------|--------|
| `ZEFC_SEL_*` literals for known names; `ZEFC_SITE` for dynamic | Same + text/reloc patch for late names under Fil-C W^X |
| Growable registry + vtable growth | Same |
| In-process `module_load` | Same ABI with `dlopen` modules later |

## Acceptance (this milestone)

1. Growable selector registry with append-only IDs — **done**.
2. Vtable growth with `doesNotUnderstand` fill — **done**.
3. Load/init-time patch of call-site cells so a send need not read a **shared** global for the ID — **done** (`ZEFC_SITE` / `patch1`).
4. ABI `patch1`: module A then B; B adds a selector; site-based sends resolve — **done** (`--suite abi`).
5. Patch representation documented above — **done**.

Instruction-count verification under Fil-C++ is a follow-on check, not a gate for landing the ABI.

## Open choices (resolved for this milestone)

- **Patch-table format:** per-site `int` cells + pending registration list (`selector_site_register` / `selector_sites_patch`).
- **First milestone module load:** explicit `module_register` / `module_load` without a separate `.so`.
- **Closed-world skip:** not implemented; sites still go through the patch path.

Next toward the ideal hot path: reloc/text-imm patch for selectors **not** in the closed-world set; **build-/link-time seal** (today’s runtime `zefc_vtables_seal()` is the stand-in); non-moving slot arena for unsealed `flat` grow; load-time `call_site_bind` coverage for monomorphic `site` sends.
