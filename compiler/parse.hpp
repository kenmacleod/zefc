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
  PackageDecl parse_package();
  Method parse_method();
  FuncDecl parse_func();
  std::vector<std::string> parse_import_path();
  std::vector<BlockItem> parse_brace_items();
  std::vector<BlockItem> parse_method_body();
  std::vector<BlockItem> parse_my_decls();
  BlockItem parse_block_item();
  ExprPtr parse_block_expr();
  ExprPtr parse_expr();
  ExprPtr parse_assign();
  ExprPtr parse_or();
  ExprPtr parse_and();
  ExprPtr parse_bitor();
  ExprPtr parse_bitxor();
  ExprPtr parse_bitand();
  ExprPtr parse_equality();
  ExprPtr parse_relational();
  ExprPtr parse_shift();
  ExprPtr parse_add();
  ExprPtr parse_mul();
  ExprPtr parse_unary();
  ExprPtr parse_postfix();
  ExprPtr parse_primary();
  ExprPtr parse_while();
  ExprPtr parse_if();
  ExprPtr parse_return();
  std::vector<ExprPtr> parse_arg_list();
};

} // namespace compiler
} // namespace zefc
