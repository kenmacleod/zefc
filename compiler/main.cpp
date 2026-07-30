#include "codegen.hpp"
#include "lexer.hpp"
#include "parse.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string
read_file(const std::string& path)
{
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("cannot open " + path);
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void
write_file(const std::string& path, const std::string& contents)
{
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("cannot write " + path);
  }
  out << contents;
}

void
usage(const char* argv0)
{
  std::cerr << "usage: " << argv0 << " <input.zef> -o <output.cpp>\n";
}

} // namespace

int
main(int argc, char** argv)
{
  try {
    std::string input;
    std::string output;
    for (int i = 1; i < argc; ++i) {
      const std::string a = argv[i];
      if (a == "-o") {
        if (i + 1 >= argc) {
          usage(argv[0]);
          return 2;
        }
        output = argv[++i];
      } else if (a == "-h" || a == "--help") {
        usage(argv[0]);
        return 0;
      } else if (input.empty()) {
        input = a;
      } else {
        usage(argv[0]);
        return 2;
      }
    }
    if (input.empty() || output.empty()) {
      usage(argv[0]);
      return 2;
    }

    const std::string src = read_file(input);
    zefc::compiler::Lexer lex(src);
    zefc::compiler::Parser parser(std::move(lex));
    zefc::compiler::Program program = parser.parse_program();
    const std::string cpp = zefc::compiler::codegen_cpp(program, input);
    write_file(output, cpp);
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "zefc: " << ex.what() << "\n";
    return 1;
  }
}
