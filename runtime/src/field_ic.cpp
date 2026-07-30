#include "zefc/field_ic.hpp"

#include "zefc/dispatch.hpp"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <utility>

namespace zefc {
namespace {

using FieldKey = std::pair<VTable*, int>;

std::map<FieldKey, FieldGetter>&
getters()
{
  static std::map<FieldKey, FieldGetter> map;
  return map;
}

std::map<FieldKey, FieldSetter>&
setters()
{
  static std::map<FieldKey, FieldSetter> map;
  return map;
}

} // namespace

void
field_register_get(VTable* vt, int selector, FieldGetter getter)
{
  if (!vt || selector <= 0 || !getter) {
    std::fprintf(stderr, "field_register_get: invalid args\n");
    std::exit(1);
  }
  getters()[FieldKey{vt, selector}] = getter;
}

void
field_register_set(VTable* vt, int selector, FieldSetter setter)
{
  if (!vt || selector <= 0 || !setter) {
    std::fprintf(stderr, "field_register_set: invalid args\n");
    std::exit(1);
  }
  setters()[FieldKey{vt, selector}] = setter;
}

id
zefc_ic_get_miss(id obj, FieldSite* site)
{
  if (!id_is_object(obj)) {
    return ZEFC_SEND0(obj, site->selector);
  }
  const auto it = getters().find(FieldKey{obj->isa_, site->selector});
  if (it != getters().end()) {
    site->guard = obj->isa_;
    site->getter = it->second;
    site->setter = nullptr;
    return site->getter(obj);
  }
  return ZEFC_SEND0(obj, site->selector);
}

id
zefc_ic_set_miss(id obj, FieldSite* site, id value)
{
  if (!id_is_object(obj)) {
    return ZEFC_SEND1(obj, site->selector, value);
  }
  const auto it = setters().find(FieldKey{obj->isa_, site->selector});
  if (it != setters().end()) {
    site->guard = obj->isa_;
    site->setter = it->second;
    site->getter = nullptr;
    return site->setter(obj, value);
  }
  return ZEFC_SEND1(obj, site->selector, value);
}

} // namespace zefc
