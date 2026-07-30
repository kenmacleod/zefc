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
    } else {
      t.kind = TokKind::Ident;
    }
    return t;
  }

  if (std::isdigit(static_cast<unsigned char>(c))) {
    std::string n;
    while (std::isdigit(static_cast<unsigned char>(ch()))) {
      n.push_back(get());
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
  case '.':
    t.kind = TokKind::Dot;
    break;
  case ',':
    t.kind = TokKind::Comma;
    break;
  case '+':
    t.kind = TokKind::Plus;
    break;
  case '=':
    if (ch() == '=') {
      get();
      t.kind = TokKind::EqEq;
    } else {
      t.kind = TokKind::Eq;
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
