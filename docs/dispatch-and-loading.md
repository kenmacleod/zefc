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

## Two different “load” stories

| Mechanism | Meaning | Status |
|-----------|---------|--------|
| **Zef `load("….zef")`** | Parse and evaluate **source** (Zef interpreter). | Needs a ZefC compiler (or temporary interpreter). Not this doc’s first milestone. |
| **ZefC package load** | Map a **compiled** module (e.g. shared object), register selectors, grow vtables, patch call sites, then run module init / top-level. | This document. |

Smoke may eventually use hand-built `.so` packages to exercise ZefC load before the compiler exists.

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

**Intent:** one-time fixup of generated code (or its reloc metadata), not an extra indirection each send.

Acceptable representations (choose one in implementation; document the choice in code comments when picked):

- Reloc / patch table emitted by the compiler: list of sites that need a selector immediate filled at load.
- Section of addressable patch cells that the **instruction stream already uses as immediates after linking** (load-time write, then execute). Prefer forms that keep the hot path at C++ cost.

**Fil-C / W^X:** patch while the text (or patch staging) is writable as required by the toolchain, then execute with normal protection. Do not assume open-ended self-modifying code after init.

**Already-loaded code that later needs a brand-new selector:** only sites that **reference** that selector need patching (e.g. a TU that imported a symbol whose ID was unknown at first link). Sites that never name the new selector stay unchanged. Prefer append-only IDs so old immediates remain valid.

## Vtable growth

When the global selector count increases:

- Expand each registered class’s `isa_` vector (or equivalent).
- Initialize new entries to `doesNotUnderstand`.
- Copy / preserve existing entries.
- Apply the loading package’s method overrides for selectors it defines.

Growth must be safe for objects already allocated: either vtables are process-global tables pointed to by `isa_`, or objects are updated according to a single documented ownership rule. Prefer one shared growable table layout pointed at by `isa_` so existing objects keep working without per-object walks when possible.

## Module layout (first milestone)

Minimum viable package unit:

- Generated C++ compiled to a **shared library** (or a second linked module registered explicitly if `dlopen` is deferred).
- Exports a module init that: registers its selectors and classes, contributes patch records, installs vtable methods.
- Host calls into the registry / loader, which runs the sequence above, then returns to the caller.

Constructor macros (`ZEFC_MODULE_CONSTRUCTOR`) may run as part of `dlopen`; ordering relative to registry init must be defined so patches run before package top-level side effects that perform sends.

## Scaffolding vs end state

| Today (smoke) | Target |
|---------------|--------|
| `extern int zefc_slot_*` loaded each send | Immediate / patched constant in the send |
| Fixed `kMaxSelectors`, manual `selector_patch` | Growable registry + automatic site patch |
| Single binary, `main` calls `runtime_package_init()` | Module load + init before package body |
| Hand-compiled cases in one exe | Same ABI, later emitted by ZefC |

## Acceptance (next implementation milestone)

1. Growable selector registry with append-only IDs.
2. Vtable growth with `doesNotUnderstand` fill.
3. Load-time (or init-time) **patch of call-site selector constants** so a send after patch does not read a global for the ID.
4. Smoke: module A loaded; module B loaded afterward; B (and A if needed) patched; a send from the documented call shape resolves correctly and matches a golden.
5. Document in this file which patch representation was chosen once implemented.

Instruction-count verification under Fil-C++ is a follow-on check, not a gate for landing the ABI.

## Open choices (resolve when implementing)

- Exact reloc / patch-table format the compiler (and hand-maintained smoke) emits.
- First milestone: `dlopen` vs explicit `zefc_register_module` without a separate `.so`.
- Whether closed-world builds skip patch tables entirely for known selectors.

Update this section when those choices are made.
