#include "lexer.hpp"

#include <cctype>
#include <stdexcept>

namespace zefc {
namespace compiler {

Lexer::Lexer(std::string source)
  : src_(std::move(source))
{
}

char
Lexer::ch() const
{
  return i_ < src_.size() ? src_[i_] : '\0';
}

char
Lexer::get()
{
  if (i_ >= src_.size()) {
    return '\0';
  }
  const char c = src_[i_++];
  if (c == '\n') {
    ++line_;
    col_ = 1;
  } else {
    ++col_;
  }
  return c;
}

void
Lexer::skip_ws_and_comments()
{
  for (;;) {
    while (std::isspace(static_cast<unsigned char>(ch()))) {
      if (ch() == '\n') {
        pending_newline_ = true;
      }
      get();
    }
    if (ch() == '#') {
      while (ch() != '\0' && ch() != '\n') {
        get();
      }
      continue;
    }
    break;
  }
}

Token
Lexer::lex_one()
{
  skip_ws_and_comments();
  Token t;
  t.line = line_;
  t.col = col_;
  t.after_newline = pending_newline_;
  pending_newline_ = false;
  const char c = ch();
  if (c == '\0') {
    t.kind = TokKind::Eof;
    return t;
  }

  if (c == '"') {
    get();
    std::string s;
    while (ch() != '\0' && ch() != '"') {
      if (ch() == '\\') {
        get();
        const char e = get();
        if (e == 'n') {
          s.push_back('\n');
        } else if (e == 't') {
          s.push_back('\t');
        } else if (e == '"' || e == '\\') {
          s.push_back(e);
        } else {
          s.push_back(e);
        }
      } else {
        s.push_back(get());
      }
    }
    if (ch() != '"') {
      throw std::runtime_error("unterminated string at " + std::to_string(t.line));
    }
    get();
    t.kind = TokKind::String;
    t.text = std::move(s);
    return t;
  }

  if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
    std::string id;
    while (std::isalnum(static_cast<unsigned char>(ch())) || ch() == '_') {
      id.push_back(get());
    }
    t.text = id;
    if (id == "class") {
      t.kind = TokKind::KwClass;
    } else if (id == "fn") {
      t.kind = TokKind::KwFn;
    } else if (id == "readable") {
      t.kind = TokKind::KwReadable;
    } else if (id == "accessible") {
      t.kind = TokKind::KwAccessible;
    } else if (id == "my") {
      t.kind = TokKind::KwMy;
    } else if (id == "static") {
      t.kind = TokKind::KwStatic;
    } else if (id == "private") {
      t.kind = TokKind::KwPrivate;
    } else if (id == "while") {
      t.kind = TokKind::KwWhile;
    } else if (id == "if") {
      t.kind = TokKind::KwIf;
    } else if (id == "else") {
      t.kind = TokKind::KwElse;
    } else if (id == "break") {
      t.kind = TokKind::KwBreak;
    } else if (id == "continue") {
      t.kind = TokKind::KwContinue;
    } else if (id == "return") {
      t.kind = TokKind::KwReturn;
    } else if (id == "package") {
      t.kind = TokKind::KwPackage;
    } else if (id == "import") {
      t.kind = TokKind::KwImport;
    } else {
      t.kind = TokKind::Ident;
    }
    return t;
  }

  if (std::isdigit(static_cast<unsigned char>(c))) {
    std::string n;
    n.push_back(get());
    if (n[0] == '0' && (ch() == 'x' || ch() == 'X')) {
      n.push_back(get());
      auto is_hex = [](char h) {
        return std::isdigit(static_cast<unsigned char>(h)) ||
               (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F');
      };
      if (!is_hex(ch())) {
        throw std::runtime_error("expected hex digit at line " + std::to_string(t.line));
      }
      while (is_hex(ch())) {
        n.push_back(get());
      }
    } else {
      while (std::isdigit(static_cast<unsigned char>(ch()))) {
        n.push_back(get());
      }
    }
    t.kind = TokKind::Number;
    t.text = std::move(n);
    return t;
  }

  get();
  switch (c) {
  case '{':
    t.kind = TokKind::LBrace;
    break;
  case '}':
    t.kind = TokKind::RBrace;
    break;
  case '(':
    t.kind = TokKind::LParen;
    break;
  case ')':
    t.kind = TokKind::RParen;
    break;
  case '[':
    t.kind = TokKind::LBracket;
    break;
  case ']':
    t.kind = TokKind::RBracket;
    break;
  case '.':
    t.kind = TokKind::Dot;
    break;
  case ',':
    t.kind = TokKind::Comma;
    break;
  case '+':
    if (ch() == '=') {
      get();
      t.kind = TokKind::PlusEq;
    } else {
      t.kind = TokKind::Plus;
    }
    break;
  case '-':
    t.kind = TokKind::Minus;
    break;
  case '*':
    if (ch() == '=') {
      get();
      t.kind = TokKind::StarEq;
    } else {
      t.kind = TokKind::Star;
    }
    break;
  case '/':
    t.kind = TokKind::Slash;
    break;
  case '%':
    t.kind = TokKind::Percent;
    break;
  case '&':
    t.kind = TokKind::Amp;
    break;
  case '=':
    if (ch() == '=') {
      get();
      t.kind = TokKind::EqEq;
    } else {
      t.kind = TokKind::Eq;
    }
    break;
  case '<':
    if (ch() == '=') {
      get();
      t.kind = TokKind::LtEq;
    } else {
      t.kind = TokKind::Lt;
    }
    break;
  case '>':
    if (ch() == '=') {
      get();
      t.kind = TokKind::GtEq;
    } else {
      t.kind = TokKind::Gt;
    }
    break;
  case ':':
    t.kind = TokKind::Colon;
    break;
  case ';':
    t.kind = TokKind::Semicolon;
    break;
  default:
    throw std::runtime_error(std::string("unexpected character '") + c + "' at line " +
                             std::to_string(t.line));
  }
  return t;
}

Token
Lexer::peek()
{
  if (!has_peek_) {
    peek_ = lex_one();
    has_peek_ = true;
  }
  return peek_;
}

Token
Lexer::next()
{
  if (has_peek_) {
    has_peek_ = false;
    return peek_;
  }
  return lex_one();
}

} // namespace compiler
} // namespace zefc
