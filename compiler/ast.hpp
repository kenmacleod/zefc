#pragma once

#include <memory>
#include <string>
#include <vector>

namespace zefc {
namespace compiler {

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct Field {
  std::string name;
  bool readable = false;
  bool accessible = false;
  bool is_static = false;
  ExprPtr init;
};

struct ClassDecl;

// Statement inside a `{ ... }` body (methods, functions, lambdas).
struct BlockItem {
  enum class Kind { Expr, VarDecl, Class } kind = Kind::Expr;
  ExprPtr expr;         // Expr stmt, or VarDecl initializer (may be null)
  std::string var_name; // VarDecl
  std::unique_ptr<ClassDecl> nested_class; // Kind::Class

  BlockItem();
  ~BlockItem();
  BlockItem(BlockItem&&) noexcept;
  BlockItem& operator=(BlockItem&&) noexcept;
  BlockItem(const BlockItem&) = delete;
  BlockItem& operator=(const BlockItem&) = delete;
};

struct Method {
  std::string name; // empty = constructor
  std::vector<std::string> params;
  std::vector<BlockItem> body; // may be empty
  bool is_static = false;
};

struct ClassDecl {
  std::string name;
  std::string emit_name; // C++ symbol prefix; empty → use name
  std::string parent; // empty if none
  std::vector<Field> fields;
  std::vector<Method> methods;
  // Type-nested classes (`static class` / `class` inside a class body).
  struct Nested {
    bool is_static = false;
    std::unique_ptr<ClassDecl> decl;
  };
  std::vector<Nested> nested;

  ClassDecl();
  ~ClassDecl();
  ClassDecl(ClassDecl&&) noexcept;
  ClassDecl& operator=(ClassDecl&&) noexcept;
  ClassDecl(const ClassDecl&) = delete;
  ClassDecl& operator=(const ClassDecl&) = delete;
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

struct FuncDecl {
  std::string name;
  std::vector<std::string> params;
  std::vector<BlockItem> body;
};

struct PackageDecl {
  std::string name;
  std::vector<Field> fields;
  std::vector<std::unique_ptr<ClassDecl>> classes;
  std::vector<FuncDecl> funcs;
  std::vector<BlockItem> ctor_body; // package constructor `fn { ... }`
  bool has_ctor = false;
  std::vector<std::unique_ptr<PackageDecl>> packages;

  PackageDecl();
  ~PackageDecl();
  PackageDecl(PackageDecl&&) noexcept;
  PackageDecl& operator=(PackageDecl&&) noexcept;
  PackageDecl(const PackageDecl&) = delete;
  PackageDecl& operator=(const PackageDecl&) = delete;
};

struct Stmt {
  enum class Kind { Class, Expr, VarDecl, Func, Package, Import } kind;
  ClassDecl class_decl;
  FuncDecl func_decl;
  PackageDecl package_decl;
  ExprPtr expr;
  std::string var_name;                 // VarDecl
  std::vector<std::string> import_path; // Import: foo or foo.bar.baz
};

struct Program {
  std::vector<Stmt> stmts;
};

} // namespace compiler
} // namespace zefc
