#include "zefc/call_ic.hpp"

#include <cstdio>
#include <cstdlib>

namespace zefc {

id
zefc_call0_miss(id obj, CallSite* site)
{
  if (!id_is_object(obj)) {
    std::fprintf(stderr, "zefc_call0_miss: not an object\n");
    std::exit(1);
  }
  zefc_method m = obj->isa_->slots[site->selector];
  site->guard = obj->isa_;
  site->callee = m;
  return m(obj, site->selector);
}

id
zefc_call1_miss(id obj, CallSite* site, id a1)
{
  if (!id_is_object(obj)) {
    std::fprintf(stderr, "zefc_call1_miss: not an object\n");
    std::exit(1);
  }
  zefc_method m = obj->isa_->slots[site->selector];
  site->guard = obj->isa_;
  site->callee = m;
  return m(obj, site->selector, a1);
}

id
zefc_call2_miss(id obj, CallSite* site, id a1, id a2)
{
  if (!id_is_object(obj)) {
    std::fprintf(stderr, "zefc_call2_miss: not an object\n");
    std::exit(1);
  }
  zefc_method m = obj->isa_->slots[site->selector];
  site->guard = obj->isa_;
  site->callee = m;
  return m(obj, site->selector, a1, a2);
}

id
zefc_call3_miss(id obj, CallSite* site, id a1, id a2, id a3)
{
  if (!id_is_object(obj)) {
    std::fprintf(stderr, "zefc_call3_miss: not an object\n");
    std::exit(1);
  }
  zefc_method m = obj->isa_->slots[site->selector];
  site->guard = obj->isa_;
  site->callee = m;
  return m(obj, site->selector, a1, a2, a3);
}

id
zefc_call4_miss(id obj, CallSite* site, id a1, id a2, id a3, id a4)
{
  if (!id_is_object(obj)) {
    std::fprintf(stderr, "zefc_call4_miss: not an object\n");
    std::exit(1);
  }
  zefc_method m = obj->isa_->slots[site->selector];
  site->guard = obj->isa_;
  site->callee = m;
  return m(obj, site->selector, a1, a2, a3, a4);
}

} // namespace zefc
