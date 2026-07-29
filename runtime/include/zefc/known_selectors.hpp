#pragma once

#include "zefc/runtime.hpp"

// Closed-world selector immediates: fixed IDs reserved at process start so
// generated/hand code can use integer literals in sends (true vtable[imm]).
// Dynamic loads still use ZEFC_SITE / selector_intern for anything beyond this set.

namespace zefc {

enum : int {
  ZEFC_SEL_toString_o = 1,
  ZEFC_SEL_add_o,
  ZEFC_SEL_sub_o,
  ZEFC_SEL_mul_o,
  ZEFC_SEL_div_o,
  ZEFC_SEL_sqrt_o,
  ZEFC_SEL_push_o,
  ZEFC_SEL_GET_i,
  ZEFC_SEL_size_o,
  ZEFC_SEL_mul_PUT_i,
  ZEFC_SEL_x_o,
  ZEFC_SEL_y_o,
  ZEFC_SEL_z_o,
  ZEFC_SEL_vx_o,
  ZEFC_SEL_vy_o,
  ZEFC_SEL_vz_o,
  ZEFC_SEL_mass_o,
  ZEFC_SEL_set_x_o,
  ZEFC_SEL_set_y_o,
  ZEFC_SEL_set_z_o,
  ZEFC_SEL_set_vx_o,
  ZEFC_SEL_set_vy_o,
  ZEFC_SEL_set_vz_o,
  ZEFC_SEL_KNOWN_END // next free ID for selector_intern
};

// Bind mangled names to the enum IDs above. Call once before other runtime inits.
void known_selectors_init();

} // namespace zefc
