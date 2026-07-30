#pragma once

#include "ast.hpp"
#include "lexer.hpp"

namespace zefc {
namespace compiler {

class Parser {
public:
  explicit Parser(Lexer lex);

  Program parse_program();

private:
  Lexer lex_;

  Token peek();
  Token next();
  bool check(TokKind k);
  Token expect(TokKind k, const char* what);

  Stmt parse_stmt();
  ClassDecl parse_class();
  Method parse_method();
  FuncDecl parse_func();
  std::vector<ExprPtr> parse_method_body();
  ExprPtr parse_expr();
  ExprPtr parse_assign();
  ExprPtr parse_equality();
  ExprPtr parse_add();
  ExprPtr parse_mul();
  ExprPtr parse_unary();
  ExprPtr parse_postfix();
  ExprPtr parse_primary();
  std::vector<ExprPtr> parse_arg_list();
};

} // namespace compiler
} // namespace zefc
