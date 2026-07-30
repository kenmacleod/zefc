#pragma once

#include <memory>
#include <string>
#include <vector>

namespace zefc {
namespace compiler {

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct Expr {
  enum class Kind {
    Ident,
    String,
    Number,
    Call,
    Dot,
    Binary,
    Assign,
  } kind;

  std::string text;              // Ident / String / Number / Dot field / Binary op
  ExprPtr lhs;                   // Dot / Binary / Assign / Call callee
  ExprPtr rhs;                   // Binary / Assign
  std::vector<ExprPtr> args;     // Call
};

struct Method {
  std::string name; // empty = constructor
  std::vector<std::string> params;
  std::vector<ExprPtr> body; // one or more statements; last yields return value
};

struct Field {
  std::string name;
  bool readable = false;
  bool accessible = false;
};

struct ClassDecl {
  std::string name;
  std::vector<Field> fields;
  std::vector<Method> methods;
};

struct Stmt {
  enum class Kind { Class, Expr } kind;
  ClassDecl class_decl;
  ExprPtr expr;
};

struct Program {
  std::vector<Stmt> stmts;
};

} // namespace compiler
} // namespace zefc
