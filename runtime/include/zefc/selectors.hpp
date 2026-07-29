#pragma once

namespace zefc {

// Patched when packages register symbols (smoke runtime uses fixed init order).
extern int zefc_slot_add_o;
extern int zefc_slot_sub_o;
extern int zefc_slot_mul_o;
extern int zefc_slot_toString_o;
extern int zefc_slot_push_o;
extern int zefc_slot_GET_i;
extern int zefc_slot_mul_PUT_i;

} // namespace zefc
