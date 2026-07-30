# Dispatch and dynamic loading

This document is the contract for ZefC’s Orchard-style dispatch under dynamic package loading. It is the next implementation focus after the smoke harness. Language overview lives in [index.md](index.md).

## Goals

- **Match C++ virtual-call cost on the hot path.** On x86-64 that is roughly three operations: load vptr, load `vtable[imm]`, `call`.
- **Allow packages to load after other TUs exist** without rebuilding every translation unit or assigning selector IDs in a closed world at compile time.
- **Preserve Zef’s “no funny business”** for objects/classes: load-time patching affects dispatch tables and call-site constants, not arbitrary object/class mutation.

## Non-goals (hot path)

- Loading a selector ID from a global, GOT, or registry **on every send**.
- String or hash lookup of method names on every send.
- Rewriting call sites continuously after they have been patched and user code is running.

Global `zefc_slot_*` variables in today’s smoke runtime are **scaffolding only**. New work must move toward the ABI below.

## Independent translation units (not blocked)

In Zef, `load("path.zef")` is a **runtime** builtin. Parsing the *caller* only builds a call with a string argument; the callee file is opened, parsed, resolved, and evaluated when that call runs. Compiling or parsing the current file does **not** require parsing the loaded file.

ZefC therefore compiles **each TU independently**. Dynamic loading is only a **runtime** concern (map a compiled module, register selectors, grow vtables, patch call-site immediates, run init). Registry / patching / multi-module smoke are **not** blocked on a ZefC source parser or on compile-on-load.

## Two different “load” stories

| Mechanism | Meaning | Blocks next milestone? |
|-----------|---------|------------------------|
| **ZefC package load** | Map a **compiled** module (e.g. shared object or registered image), register selectors, grow vtables, patch call sites, then run module init / top-level. | **No** — this is the next work; hand-built modules are enough. |
| **Zef `load("….zef")` as source** | Interpreter: parse and evaluate source at the call. | **No** for dispatch work. Later: either AOT (`load` resolves to a prebuilt module on a search path) or compile-on-load once the compiler exists. |

Smoke may use hand-built `.so` (or in-process `module_register` / `module_load`) packages to exercise ZefC load before the compiler exists. Faithful Zef `load*.zef` tests that pass a **source** path can wait for AOT packaging or the compiler; they do not gate the dispatch ABI. In-process registration of hand-compiled modules is enough to prove load sequencing and package bindings.

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

**Closed-world immediates (landed):** well-known mangled names are reserved at process start via `selector_reserve` / `known_selectors_init()` into fixed `ZEFC_SEL_*` enum IDs (`known_selectors.hpp`). Hand-compiled or generated code may pass those **integer literals** into `ZEFC_SEND*`, so the compiler can emit `vtable[imm]` with no cell load. `ZEFC_SITE` remains for late/dynamic names (`patch1`, open-ended modules).

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

| Today (smoke) | Target |
|---------------|--------|
| `ZEFC_SEL_*` literals for known names; `ZEFC_SITE` for dynamic | Same + text/reloc patch for late names under Fil-C W^X |
| Growable registry + vtable growth | Same |
| In-process `module_load` | Same ABI with `dlopen` modules later |

## Acceptance (this milestone)

1. Growable selector registry with append-only IDs — **done**.
2. Vtable growth with `doesNotUnderstand` fill — **done**.
3. Load/init-time patch of call-site cells so a send need not read a **shared** global for the ID — **done** (`ZEFC_SITE` / `patch1`).
4. Smoke `patch1`: module A then B; B adds a selector; site-based sends resolve — **done**.
5. Patch representation documented above — **done**.

Instruction-count verification under Fil-C++ is a follow-on check, not a gate for landing the ABI.

## Open choices (resolved for this milestone)

- **Patch-table format:** per-site `int` cells + pending registration list (`selector_site_register` / `selector_sites_patch`).
- **First milestone module load:** explicit `module_register` / `module_load` without a separate `.so`.
- **Closed-world skip:** not implemented; sites still go through the patch path.

Next toward the ideal hot path: reloc/text-imm patch for selectors **not** in the closed-world set; optionally flatten `isa_` back to `zefc_method*` with a non-moving slot allocator.

**Monomorphic fields (future compiler):** when analysis proves a fixed receiver class, accessible get/set may lower to direct struct access while vtable methods remain for polymorphic sends. Smoke does **not** apply that by hand — `nbody` uses field sends so the bench matches general transpile shape.
