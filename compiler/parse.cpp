#include "parse.hpp"

#include <stdexcept>

namespace zefc {
namespace compiler {

Parser::Parser(Lexer lex)
  : lex_(std::move(lex))
{
}

Token
Parser::peek()
{
  return lex_.peek();
}

Token
Parser::next()
{
  return lex_.next();
}

bool
Parser::check(TokKind k)
{
  return peek().kind == k;
}

Token
Parser::expect(TokKind k, const char* what)
{
  Token t = next();
  if (t.kind != k) {
    throw std::runtime_error(std::string("expected ") + what + " at line " +
                             std::to_string(t.line));
  }
  return t;
}

Program
Parser::parse_program()
{
  Program p;
  while (!check(TokKind::Eof)) {
    // Optional statement terminators (Zef allows `;`).
    while (check(TokKind::Semicolon)) {
      next();
    }
    if (check(TokKind::Eof)) {
      break;
    }
    p.stmts.push_back(parse_stmt());
    while (check(TokKind::Semicolon)) {
      next();
    }
  }
  return p;
}

Stmt
Parser::parse_stmt()
{
  if (check(TokKind::KwClass)) {
    Stmt s;
    s.kind = Stmt::Kind::Class;
    s.class_decl = parse_class();
    return s;
  }
  if (check(TokKind::KwFn)) {
    // Top-level named function vs expression starting with fn — named has Ident.
    Token save = peek();
    (void)save;
    // parse_func requires name; but `fn (...)` at top level is rare. Peek ahead:
    next(); // fn
    if (check(TokKind::Ident)) {
      // Rewind by re-parsing: we consumed fn. Build FuncDecl manually.
      FuncDecl f;
      f.name = next().text;
      if (check(TokKind::LParen)) {
        next();
        if (!check(TokKind::RParen)) {
          f.params.push_back(expect(TokKind::Ident, "parameter").text);
          while (check(TokKind::Comma)) {
            next();
            f.params.push_back(expect(TokKind::Ident, "parameter").text);
          }
        }
        expect(TokKind::RParen, ")");
      }
      f.body = parse_method_body();
      Stmt s;
      s.kind = Stmt::Kind::Func;
      s.func_decl = std::move(f);
      return s;
    }
    // Anonymous fn expression at top level — rebuild Lambda.
    auto lam = std::make_unique<Expr>();
    lam->kind = Expr::Kind::Lambda;
    if (check(TokKind::LParen)) {
      next();
      if (!check(TokKind::RParen)) {
        lam->params.push_back(expect(TokKind::Ident, "parameter").text);
        while (check(TokKind::Comma)) {
          next();
          lam->params.push_back(expect(TokKind::Ident, "parameter").text);
        }
      }
      expect(TokKind::RParen, ")");
    }
    lam->body = parse_method_body();
    Stmt s;
    s.kind = Stmt::Kind::Expr;
    s.expr = std::move(lam);
    return s;
  }
  // Top-level: my name [= expr]
  if (check(TokKind::KwMy)) {
    next();
    Stmt s;
    s.kind = Stmt::Kind::VarDecl;
    s.var_name = expect(TokKind::Ident, "variable name").text;
    if (check(TokKind::Eq)) {
      next();
      s.expr = parse_expr();
    }
    return s;
  }
  if (check(TokKind::KwWhile)) {
    Stmt s;
    s.kind = Stmt::Kind::Expr;
    s.expr = parse_while();
    return s;
  }
  if (check(TokKind::KwIf)) {
    Stmt s;
    s.kind = Stmt::Kind::Expr;
    s.expr = parse_if();
    return s;
  }
  if (check(TokKind::KwBreak) || check(TokKind::KwContinue)) {
    auto e = std::make_unique<Expr>();
    e->kind = check(TokKind::KwBreak) ? Expr::Kind::Break : Expr::Kind::Continue;
    next();
    Stmt s;
    s.kind = Stmt::Kind::Expr;
    s.expr = std::move(e);
    return s;
  }
  Stmt s;
  s.kind = Stmt::Kind::Expr;
  s.expr = parse_expr();
  return s;
}

ClassDecl
Parser::parse_class()
{
  expect(TokKind::KwClass, "class");
  ClassDecl c;
  c.name = expect(TokKind::Ident, "class name").text;
  if (check(TokKind::Colon)) {
    next();
    c.parent = expect(TokKind::Ident, "parent class").text;
  }
  expect(TokKind::LBrace, "{");
  while (!check(TokKind::RBrace) && !check(TokKind::Eof)) {
    bool is_static = false;
    if (check(TokKind::KwStatic)) {
      next();
      is_static = true;
    }
    if (check(TokKind::KwReadable) || check(TokKind::KwAccessible) || check(TokKind::KwMy)) {
      const bool is_my = check(TokKind::KwMy);
      const bool is_acc = check(TokKind::KwAccessible);
      next();
      for (;;) {
        Field f;
        f.is_static = is_static;
        f.name = expect(TokKind::Ident, "field name").text;
        if (is_my) {
        } else if (is_acc) {
          f.accessible = true;
          f.readable = true;
        } else {
          f.readable = true;
        }
        if (check(TokKind::Eq)) {
          next();
          f.init = parse_expr();
        }
        c.fields.push_back(std::move(f));
        if (check(TokKind::Comma)) {
          next();
          continue;
        }
        break;
      }
    } else if (check(TokKind::KwFn)) {
      if (is_static) {
        throw std::runtime_error("static methods not supported yet");
      }
      c.methods.push_back(parse_method());
    } else {
      Token t = peek();
      throw std::runtime_error("unexpected token in class body at line " +
                               std::to_string(t.line));
    }
  }
  expect(TokKind::RBrace, "}");
  return c;
}

Method
Parser::parse_method()
{
  expect(TokKind::KwFn, "fn");
  Method m;
  if (check(TokKind::Ident)) {
    m.name = next().text;
  }
  if (check(TokKind::LParen)) {
    next();
    if (!check(TokKind::RParen)) {
      m.params.push_back(expect(TokKind::Ident, "parameter").text);
      while (check(TokKind::Comma)) {
        next();
        m.params.push_back(expect(TokKind::Ident, "parameter").text);
      }
    }
    expect(TokKind::RParen, ")");
  }
  m.body = parse_method_body();
  return m;
}

FuncDecl
Parser::parse_func()
{
  expect(TokKind::KwFn, "fn");
  FuncDecl f;
  f.name = expect(TokKind::Ident, "function name").text;
  if (check(TokKind::LParen)) {
    next();
    if (!check(TokKind::RParen)) {
      f.params.push_back(expect(TokKind::Ident, "parameter").text);
      while (check(TokKind::Comma)) {
        next();
        f.params.push_back(expect(TokKind::Ident, "parameter").text);
      }
    }
    expect(TokKind::RParen, ")");
  }
  f.body = parse_method_body();
  return f;
}

std::vector<BlockItem>
Parser::parse_method_body()
{
  std::vector<BlockItem> body;
  if (check(TokKind::LBrace)) {
    next();
    while (!check(TokKind::RBrace) && !check(TokKind::Eof)) {
      while (check(TokKind::Semicolon)) {
        next();
      }
      if (check(TokKind::RBrace)) {
        break;
      }
      body.push_back(parse_block_item());
      while (check(TokKind::Semicolon)) {
        next();
      }
    }
    expect(TokKind::RBrace, "}");
    return body; // may be empty
  }
  body.push_back(parse_block_item());
  return body;
}

BlockItem
Parser::parse_block_item()
{
  // Local: my name [= expr]
  if (check(TokKind::KwMy)) {
    next();
    BlockItem item;
    item.kind = BlockItem::Kind::VarDecl;
    item.var_name = expect(TokKind::Ident, "variable name").text;
    if (check(TokKind::Eq)) {
      next();
      item.expr = parse_expr();
    }
    return item;
  }
  // Named local function: fn name(...) body  →  my name = fn (...) body
  if (check(TokKind::KwFn)) {
    next(); // consume fn
    if (check(TokKind::Ident)) {
      BlockItem item;
      item.kind = BlockItem::Kind::VarDecl;
      item.var_name = next().text;
      auto lam = std::make_unique<Expr>();
      lam->kind = Expr::Kind::Lambda;
      if (check(TokKind::LParen)) {
        next();
        if (!check(TokKind::RParen)) {
          lam->params.push_back(expect(TokKind::Ident, "parameter").text);
          while (check(TokKind::Comma)) {
            next();
            lam->params.push_back(expect(TokKind::Ident, "parameter").text);
          }
        }
        expect(TokKind::RParen, ")");
      }
      lam->body = parse_method_body();
      item.expr = std::move(lam);
      return item;
    }
    // Anonymous lambda as expression statement.
    auto lam = std::make_unique<Expr>();
    lam->kind = Expr::Kind::Lambda;
    if (check(TokKind::LParen)) {
      next();
      if (!check(TokKind::RParen)) {
        lam->params.push_back(expect(TokKind::Ident, "parameter").text);
        while (check(TokKind::Comma)) {
          next();
          lam->params.push_back(expect(TokKind::Ident, "parameter").text);
        }
      }
      expect(TokKind::RParen, ")");
    }
    lam->body = parse_method_body();
    BlockItem item;
    item.kind = BlockItem::Kind::Expr;
    item.expr = std::move(lam);
    return item;
  }
  if (check(TokKind::KwWhile)) {
    BlockItem item;
    item.kind = BlockItem::Kind::Expr;
    item.expr = parse_while();
    return item;
  }
  if (check(TokKind::KwIf)) {
    BlockItem item;
    item.kind = BlockItem::Kind::Expr;
    item.expr = parse_if();
    return item;
  }
  if (check(TokKind::KwBreak) || check(TokKind::KwContinue)) {
    auto e = std::make_unique<Expr>();
    e->kind = check(TokKind::KwBreak) ? Expr::Kind::Break : Expr::Kind::Continue;
    next();
    BlockItem item;
    item.kind = BlockItem::Kind::Expr;
    item.expr = std::move(e);
    return item;
  }
  BlockItem item;
  item.kind = BlockItem::Kind::Expr;
  item.expr = parse_expr();
  return item;
}

ExprPtr
Parser::parse_while()
{
  expect(TokKind::KwWhile, "while");
  expect(TokKind::LParen, "(");
  auto e = std::make_unique<Expr>();
  e->kind = Expr::Kind::While;
  e->lhs = parse_expr();
  expect(TokKind::RParen, ")");
  e->body = parse_method_body();
  return e;
}

ExprPtr
Parser::parse_if()
{
  expect(TokKind::KwIf, "if");
  expect(TokKind::LParen, "(");
  auto e = std::make_unique<Expr>();
  e->kind = Expr::Kind::If;
  e->lhs = parse_expr();
  expect(TokKind::RParen, ")");
  e->body = parse_method_body();
  return e;
}

ExprPtr
Parser::parse_expr()
{
  return parse_assign();
}

ExprPtr
Parser::parse_assign()
{
  ExprPtr e = parse_equality();
  if (check(TokKind::Eq) || check(TokKind::PlusEq)) {
    const bool plus_eq = check(TokKind::PlusEq);
    next();
    auto a = std::make_unique<Expr>();
    a->kind = Expr::Kind::Assign;
    a->text = plus_eq ? "+=" : "=";
    a->lhs = std::move(e);
    a->rhs = parse_assign();
    return a;
  }
  return e;
}

ExprPtr
Parser::parse_equality()
{
  ExprPtr e = parse_relational();
  while (check(TokKind::EqEq)) {
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = "==";
    b->lhs = std::move(e);
    b->rhs = parse_relational();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_relational()
{
  ExprPtr e = parse_add();
  for (;;) {
    std::string op;
    if (check(TokKind::Lt)) {
      op = "<";
    } else if (check(TokKind::LtEq)) {
      op = "<=";
    } else if (check(TokKind::Gt)) {
      op = ">";
    } else if (check(TokKind::GtEq)) {
      op = ">=";
    } else {
      break;
    }
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = op;
    b->lhs = std::move(e);
    b->rhs = parse_add();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_add()
{
  ExprPtr e = parse_mul();
  for (;;) {
    // Newline before +/- starts a new statement (so `x = 42\n- 666` is two stmts).
    if (peek().after_newline && (check(TokKind::Plus) || check(TokKind::Minus))) {
      break;
    }
    if (!(check(TokKind::Plus) || check(TokKind::Minus))) {
      break;
    }
    const bool is_minus = check(TokKind::Minus);
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = is_minus ? "-" : "+";
    b->lhs = std::move(e);
    b->rhs = parse_mul();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_mul()
{
  ExprPtr e = parse_unary();
  while (check(TokKind::Star) || check(TokKind::Slash) || check(TokKind::Percent)) {
    std::string op = "*";
    if (check(TokKind::Slash)) {
      op = "/";
    } else if (check(TokKind::Percent)) {
      op = "%";
    }
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = op;
    b->lhs = std::move(e);
    b->rhs = parse_unary();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_unary()
{
  if (check(TokKind::Minus)) {
    next();
    auto u = std::make_unique<Expr>();
    u->kind = Expr::Kind::Unary;
    u->text = "-";
    u->rhs = parse_unary();
    return u;
  }
  return parse_postfix();
}

ExprPtr
Parser::parse_postfix()
{
  ExprPtr e = parse_primary();
  for (;;) {
    if (check(TokKind::Dot)) {
      next();
      auto d = std::make_unique<Expr>();
      d->kind = Expr::Kind::Dot;
      d->lhs = std::move(e);
      d->text = expect(TokKind::Ident, "member name").text;
      e = std::move(d);
    } else if (check(TokKind::LParen)) {
      auto c = std::make_unique<Expr>();
      c->kind = Expr::Kind::Call;
      c->lhs = std::move(e);
      c->args = parse_arg_list();
      e = std::move(c);
    } else {
      break;
    }
  }
  return e;
}

ExprPtr
Parser::parse_primary()
{
  if (check(TokKind::KwFn)) {
    next();
    auto e = std::make_unique<Expr>();
    e->kind = Expr::Kind::Lambda;
    // Optional params; otherwise expression/block body (e.g. `fn println("hi")`).
    if (check(TokKind::LParen)) {
      next();
      if (!check(TokKind::RParen)) {
        e->params.push_back(expect(TokKind::Ident, "parameter").text);
        while (check(TokKind::Comma)) {
          next();
          e->params.push_back(expect(TokKind::Ident, "parameter").text);
        }
      }
      expect(TokKind::RParen, ")");
    }
    e->body = parse_method_body();
    return e;
  }
  if (check(TokKind::Ident)) {
    auto e = std::make_unique<Expr>();
    e->kind = Expr::Kind::Ident;
    e->text = next().text;
    return e;
  }
  if (check(TokKind::String)) {
    auto e = std::make_unique<Expr>();
    e->kind = Expr::Kind::String;
    e->text = next().text;
    return e;
  }
  if (check(TokKind::Number)) {
    auto e = std::make_unique<Expr>();
    e->kind = Expr::Kind::Number;
    e->text = next().text;
    return e;
  }
  if (check(TokKind::LParen)) {
    next();
    ExprPtr e = parse_expr();
    expect(TokKind::RParen, ")");
    return e;
  }
  Token t = peek();
  throw std::runtime_error("expected expression at line " + std::to_string(t.line));
}

std::vector<ExprPtr>
Parser::parse_arg_list()
{
  expect(TokKind::LParen, "(");
  std::vector<ExprPtr> args;
  if (!check(TokKind::RParen)) {
    args.push_back(parse_expr());
    while (check(TokKind::Comma)) {
      next();
      args.push_back(parse_expr());
    }
  }
  expect(TokKind::RParen, ")");
  return args;
}

} // namespace compiler
} // namespace zefc
