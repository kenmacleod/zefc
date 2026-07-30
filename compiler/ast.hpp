#pragma once

#include <memory>
#include <string>
#include <vector>

namespace zefc {
namespace compiler {

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

// Statement inside a `{ ... }` body (methods, functions, lambdas).
struct BlockItem {
  enum class Kind { Expr, VarDecl } kind;
  ExprPtr expr;         // Expr stmt, or VarDecl initializer (may be null)
  std::string var_name; // VarDecl
};

struct Expr {
  enum class Kind {
    Ident,
    String,
    Number,
    Call,
    Dot,
    Index,
    ArrayLit,
    Binary,
    Unary,
    Assign,
    Lambda,
    While,
    If,
    Break,
    Continue,
    Return,
  } kind;

  std::string text; // Ident / String / Number / Dot field / Binary op / Assign "+="
  ExprPtr lhs;
  ExprPtr rhs;
  std::vector<ExprPtr> args;
  std::vector<std::string> params; // Lambda
  std::vector<BlockItem> body;      // Lambda / While / If-then
  std::vector<BlockItem> else_body; // If-else
};

struct Field {
  std::string name;
  bool readable = false;
  bool accessible = false;
  bool is_static = false;
  ExprPtr init;
};

struct Method {
  std::string name; // empty = constructor
  std::vector<std::string> params;
  std::vector<BlockItem> body; // may be empty
  bool is_static = false;
};

struct ClassDecl {
  std::string name;
  std::string parent; // empty if none
  std::vector<Field> fields;
  std::vector<Method> methods;
};

struct FuncDecl {
  std::string name;
  std::vector<std::string> params;
  std::vector<BlockItem> body;
};

struct Stmt {
  enum class Kind { Class, Expr, VarDecl, Func } kind;
  ClassDecl class_decl;
  FuncDecl func_decl;
  ExprPtr expr;
  std::string var_name;
};

struct Program {
  std::vector<Stmt> stmts;
};

} // namespace compiler
} // namespace zefc
