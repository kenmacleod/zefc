#include "zefc/known_selectors.hpp"

namespace zefc {

void
known_selectors_init()
{
  selector_reserve("toString_o", ZEFC_SEL_toString_o);
  selector_reserve("add_o", ZEFC_SEL_add_o);
  selector_reserve("sub_o", ZEFC_SEL_sub_o);
  selector_reserve("mul_o", ZEFC_SEL_mul_o);
  selector_reserve("div_o", ZEFC_SEL_div_o);
  selector_reserve("sqrt_o", ZEFC_SEL_sqrt_o);
  selector_reserve("push_o", ZEFC_SEL_push_o);
  selector_reserve("GET_i", ZEFC_SEL_GET_i);
  selector_reserve("size_o", ZEFC_SEL_size_o);
  selector_reserve("mul_PUT_i", ZEFC_SEL_mul_PUT_i);
  selector_reserve("x_o", ZEFC_SEL_x_o);
  selector_reserve("y_o", ZEFC_SEL_y_o);
  selector_reserve("z_o", ZEFC_SEL_z_o);
  selector_reserve("vx_o", ZEFC_SEL_vx_o);
  selector_reserve("vy_o", ZEFC_SEL_vy_o);
  selector_reserve("vz_o", ZEFC_SEL_vz_o);
  selector_reserve("mass_o", ZEFC_SEL_mass_o);
  selector_reserve("set_x_o", ZEFC_SEL_set_x_o);
  selector_reserve("set_y_o", ZEFC_SEL_set_y_o);
  selector_reserve("set_z_o", ZEFC_SEL_set_z_o);
  selector_reserve("set_vx_o", ZEFC_SEL_set_vx_o);
  selector_reserve("set_vy_o", ZEFC_SEL_set_vy_o);
  selector_reserve("set_vz_o", ZEFC_SEL_set_vz_o);
}

} // namespace zefc
