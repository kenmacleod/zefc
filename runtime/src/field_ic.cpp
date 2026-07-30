#include "zefc/field_ic.hpp"

#include "zefc/dispatch.hpp"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <utility>

namespace zefc {
namespace {

using FieldKey = std::pair<VTable*, int>;

std::map<FieldKey, std::intptr_t>&
get_offsets()
{
  static std::map<FieldKey, std::intptr_t> map;
  return map;
}

std::map<FieldKey, std::intptr_t>&
set_offsets()
{
  static std::map<FieldKey, std::intptr_t> map;
  return map;
}

} // namespace

void
field_register_get(VTable* vt, int selector, std::size_t offset)
{
  if (!vt || selector <= 0) {
    std::fprintf(stderr, "field_register_get: invalid args\n");
    std::exit(1);
  }
  get_offsets()[FieldKey{vt, selector}] = static_cast<std::intptr_t>(offset);
}

void
field_register_set(VTable* vt, int selector, std::size_t offset)
{
  if (!vt || selector <= 0) {
    std::fprintf(stderr, "field_register_set: invalid args\n");
    std::exit(1);
  }
  set_offsets()[FieldKey{vt, selector}] = static_cast<std::intptr_t>(offset);
}

bool
field_lookup_get(VTable* vt, int selector, std::intptr_t* out_offset)
{
  const auto it = get_offsets().find(FieldKey{vt, selector});
  if (it == get_offsets().end()) {
    return false;
  }
  *out_offset = it->second;
  return true;
}

bool
field_lookup_set(VTable* vt, int selector, std::intptr_t* out_offset)
{
  const auto it = set_offsets().find(FieldKey{vt, selector});
  if (it == set_offsets().end()) {
    return false;
  }
  *out_offset = it->second;
  return true;
}

id
zefc_ic_get_miss_send(id obj, FieldSite* site)
{
  return ZEFC_SEND0(obj, site->selector);
}

id
zefc_ic_set_miss_send(id obj, FieldSite* site, id value)
{
  return ZEFC_SEND1(obj, site->selector, value);
}

id
zefc_ic_get_offset_miss(id obj, FieldSite* site)
{
  if (!id_is_object(obj)) {
    return zefc_ic_get_miss_send(obj, site);
  }
  std::intptr_t off = 0;
  if (!field_lookup_get(zefc_vtable_of(obj), site->selector, &off)) {
    return zefc_ic_get_miss_send(obj, site);
  }
  site->guard = obj->isa_;
  site->offset = off;
  return *field_slot(obj, site->offset);
}

id
zefc_ic_set_offset_miss(id obj, FieldSite* site, id value)
{
  if (!id_is_object(obj)) {
    return zefc_ic_set_miss_send(obj, site, value);
  }
  std::intptr_t off = 0;
  if (!field_lookup_set(zefc_vtable_of(obj), site->selector, &off)) {
    return zefc_ic_set_miss_send(obj, site, value);
  }
  site->guard = obj->isa_;
  site->offset = off;
  *field_slot(obj, site->offset) = value;
  return null_id();
}

} // namespace zefc
