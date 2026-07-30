#pragma once

#include "zefc/runtime.hpp"

#include <initializer_list>

namespace zefc {

id Array__from_ints(std::initializer_list<long long> values);
id Array__new();
id Array__with_size(int n); // n slots, filled with null
id Array__push(id array, id value);
id Array__at(id array, int index);
void Array__set_at(id array, int index, id value);
int Array__size(id array);
void Array__mul_assign_at(id array, int index, long long factor);

} // namespace zefc
