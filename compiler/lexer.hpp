#pragma once

#include <string>
#include <vector>

namespace zefc {
namespace compiler {

enum class TokKind {
  Eof,
  Ident,
  String,
  Number,
  // keywords
  KwClass,
  KwFn,
  KwReadable,
  KwAccessible,
  KwMy,
  KwStatic,
  KwPrivate,
  KwWhile,
  KwIf,
  KwElse,
  KwBreak,
  KwContinue,
  KwReturn,
  // punct
  LBrace,
  RBrace,
  LParen,
  RParen,
  LBracket,
  RBracket,
  Dot,
  Comma,
  Plus,
  PlusEq,
  Minus,
  Star,
  StarEq,
  Slash,
  Percent,
  Amp, // &
  Eq,
  EqEq,
  Lt,
  LtEq,
  Gt,
  GtEq,
  Colon,
  Semicolon,
};

struct Token {
  TokKind kind = TokKind::Eof;
  std::string text;
  int line = 1;
  int col = 1;
  bool after_newline = false; // true if whitespace before this token included a newline
};

class Lexer {
public:
  explicit Lexer(std::string source);

  Token peek();
  Token next();

private:
  std::string src_;
  size_t i_ = 0;
  int line_ = 1;
  int col_ = 1;
  bool has_peek_ = false;
  Token peek_;

  char ch() const;
  char get();
  void skip_ws_and_comments();
  Token lex_one();

  bool pending_newline_ = false;
};

} // namespace compiler
} // namespace zefc
