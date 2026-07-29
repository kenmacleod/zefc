#pragma once

namespace zefc {

// Compat globals: patched once at runtime init. Prefer ZEFC_SITE at new
// call sites so each site has its own cell (path toward imm patching).
extern int zefc_slot_add_o;
extern int zefc_slot_sub_o;
extern int zefc_slot_mul_o;
extern int zefc_slot_toString_o;
extern int zefc_slot_push_o;
extern int zefc_slot_GET_i;
extern int zefc_slot_mul_PUT_i;

} // namespace zefc
