#pragma once

// Object ZEFC_SEND* policy (Meson -Dobject_dispatch=… → -DZEFC_OBJECT_DISPATCH=N).
//
//   slots (0): isa_->slots[sel] every send (VTable* handle)
//   ic    (1): per-site guarded method IC (default)
//   flat  (2): isa_ is zefc_method*; send isa_[sel]; grow may fix up instances
//   site  (3): per-site sticky callee (fill on first use; no isa_ guard)
//
#ifndef ZEFC_OBJECT_DISPATCH
#define ZEFC_OBJECT_DISPATCH 1
#endif

#define ZEFC_OD_SLOTS 0
#define ZEFC_OD_IC 1
#define ZEFC_OD_FLAT 2
#define ZEFC_OD_SITE 3
