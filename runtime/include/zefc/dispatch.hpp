#pragma once

#include "zefc/runtime.hpp"
#include "zefc/selectors.hpp"

namespace zefc {

#define ZEFC_SEND0(obj, slot) \
  ((obj)->isa_[(slot)]((obj), (slot)))

#define ZEFC_SEND1(obj, slot, a1) \
  ((obj)->isa_[(slot)]((obj), (slot), (a1)))

#define ZEFC_SEND2(obj, slot, a1, a2) \
  ((obj)->isa_[(slot)]((obj), (slot), (a1), (a2)))

inline id send(id recv, int slot, id arg0)
{
  return ZEFC_SEND1(recv, slot, arg0);
}

} // namespace zefc
