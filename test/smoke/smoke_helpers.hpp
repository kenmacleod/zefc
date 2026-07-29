#pragma once

#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/string_api.hpp"

#include <cstdio>

namespace zefc {
namespace smoke {

inline void
println_fx(const char* prefix, long long f, long long x)
{
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%s f = %lld, x = %lld", prefix, f, x);
  println(String__from_utf8(buf));
}

} // namespace smoke
} // namespace zefc
