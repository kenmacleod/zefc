#pragma once

#include "ast.hpp"

#include <string>

namespace zefc {
namespace compiler {

// Lower Program to standalone C++ that links zefc runtime + stdlib bootstrap.
// Uses ZEFC_SEND* / ZEFC_IC_* / ZEFC_SEL_* so -Dobject_dispatch still applies.
std::string codegen_cpp(const Program& program, const std::string& source_path);

} // namespace compiler
} // namespace zefc
