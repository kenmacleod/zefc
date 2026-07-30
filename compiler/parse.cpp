#include "parse.hpp"

#include <stdexcept>
#include <utility>

namespace zefc {
namespace compiler {

BlockItem::BlockItem() = default;
BlockItem::~BlockItem() = default;
BlockItem::BlockItem(BlockItem&&) noexcept = default;
BlockItem& BlockItem::operator=(BlockItem&&) noexcept = default;

ClassDecl::ClassDecl() = default;
ClassDecl::~ClassDecl() = default;
ClassDecl::ClassDecl(ClassDecl&&) noexcept = default;
ClassDecl& ClassDecl::operator=(ClassDecl&&) noexcept = default;

PackageDecl::PackageDecl() = default;
PackageDecl::~PackageDecl() = default;
PackageDecl::PackageDecl(PackageDecl&&) noexcept = default;
PackageDecl& PackageDecl::operator=(PackageDecl&&) noexcept = default;

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
  const int line = peek().line;
  if (check(TokKind::KwPackage)) {
    Stmt s;
    s.kind = Stmt::Kind::Package;
    s.line = line;
    s.package_decl = parse_package();
    return s;
  }
  if (check(TokKind::KwImport)) {
    next();
    Stmt s;
    s.kind = Stmt::Kind::Import;
    s.line = line;
    s.import_path.push_back(expect(TokKind::Ident, "package name").text);
    while (check(TokKind::Dot)) {
      next();
      s.import_path.push_back(expect(TokKind::Ident, "package name").text);
    }
    return s;
  }
  if (check(TokKind::KwClass)) {
    Stmt s;
    s.kind = Stmt::Kind::Class;
    s.line = line;
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
      f.line = line;
      f.body = parse_method_body();
      Stmt s;
      s.kind = Stmt::Kind::Func;
      s.line = line;
      s.func_decl = std::move(f);
      return s;
    }
    // Anonymous fn expression at top level — rebuild Lambda.
    auto lam = std::make_unique<Expr>();
    lam->kind = Expr::Kind::Lambda;
    lam->line = line;
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
    s.line = line;
    s.expr = std::move(lam);
    return s;
  }
  // Top-level: my name [= expr]
  if (check(TokKind::KwMy)) {
    next();
    Stmt s;
    s.kind = Stmt::Kind::VarDecl;
    s.line = line;
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
    s.line = line;
    s.expr = parse_while();
    return s;
  }
  if (check(TokKind::KwIf)) {
    Stmt s;
    s.kind = Stmt::Kind::Expr;
    s.line = line;
    s.expr = parse_if();
    return s;
  }
  if (check(TokKind::KwBreak) || check(TokKind::KwContinue)) {
    auto e = std::make_unique<Expr>();
    e->kind = check(TokKind::KwBreak) ? Expr::Kind::Break : Expr::Kind::Continue;
    e->line = line;
    next();
    Stmt s;
    s.kind = Stmt::Kind::Expr;
    s.line = line;
    s.expr = std::move(e);
    return s;
  }
  if (check(TokKind::KwReturn)) {
    Stmt s;
    s.kind = Stmt::Kind::Expr;
    s.line = line;
    s.expr = parse_return();
    return s;
  }
  Stmt s;
  s.kind = Stmt::Kind::Expr;
  s.line = line;
  s.expr = parse_expr();
  return s;
}

ClassDecl
Parser::parse_class()
{
  const int line = peek().line;
  expect(TokKind::KwClass, "class");
  ClassDecl c;
  c.line = line;
  c.name = expect(TokKind::Ident, "class name").text;
  if (check(TokKind::Colon)) {
    next();
    c.parent = expect(TokKind::Ident, "parent class").text;
  }
  expect(TokKind::LBrace, "{");
  while (!check(TokKind::RBrace) && !check(TokKind::Eof)) {
    bool is_static = false;
    bool is_private = false;
    if (check(TokKind::KwStatic)) {
      next();
      is_static = true;
    }
    if (check(TokKind::KwPrivate)) {
      next();
      is_private = true;
    }
    // Allow `private static` / `static private` either order.
    if (!is_static && check(TokKind::KwStatic)) {
      next();
      is_static = true;
    }
    if (!is_private && check(TokKind::KwPrivate)) {
      next();
      is_private = true;
    }
    (void)is_private; // access checks not enforced in transpiler yet
    if (check(TokKind::KwClass)) {
      ClassDecl::Nested nc;
      nc.is_static = is_static;
      nc.decl = std::make_unique<ClassDecl>(parse_class());
      c.nested.push_back(std::move(nc));
    } else if (check(TokKind::KwReadable) || check(TokKind::KwAccessible) || check(TokKind::KwMy)) {
      const bool is_my = check(TokKind::KwMy);
      const bool is_acc = check(TokKind::KwAccessible);
      next();
      for (;;) {
        Field f;
        f.line = peek().line;
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
      Method m = parse_method();
      m.is_static = is_static;
      c.methods.push_back(std::move(m));
    } else {
      Token t = peek();
      throw std::runtime_error("unexpected token in class body at line " +
                               std::to_string(t.line));
    }
  }
  expect(TokKind::RBrace, "}");
  return c;
}

PackageDecl
Parser::parse_package()
{
  const int line = peek().line;
  expect(TokKind::KwPackage, "package");
  std::vector<std::string> path;
  path.push_back(expect(TokKind::Ident, "package name").text);
  while (check(TokKind::Dot)) {
    next();
    path.push_back(expect(TokKind::Ident, "package name").text);
  }
  expect(TokKind::LBrace, "{");

  PackageDecl body;
  while (!check(TokKind::RBrace) && !check(TokKind::Eof)) {
    if (check(TokKind::KwPackage)) {
      body.packages.push_back(std::make_unique<PackageDecl>(parse_package()));
    } else if (check(TokKind::KwPrivate) || check(TokKind::KwStatic) || check(TokKind::KwClass)) {
      // `private` / `static` before nested class (access not enforced yet).
      while (check(TokKind::KwPrivate) || check(TokKind::KwStatic)) {
        next();
      }
      body.classes.push_back(std::make_unique<ClassDecl>(parse_class()));
    } else if (check(TokKind::KwReadable) || check(TokKind::KwAccessible) ||
               check(TokKind::KwMy)) {
      const bool is_my = check(TokKind::KwMy);
      const bool is_acc = check(TokKind::KwAccessible);
      next();
      for (;;) {
        Field f;
        f.line = peek().line;
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
        body.fields.push_back(std::move(f));
        if (check(TokKind::Comma)) {
          next();
          continue;
        }
        break;
      }
    } else if (check(TokKind::KwFn)) {
      Method m = parse_method();
      if (m.name.empty()) {
        if (body.has_ctor) {
          throw std::runtime_error("duplicate package constructor");
        }
        body.has_ctor = true;
        body.ctor_body = std::move(m.body);
      } else {
        FuncDecl f;
        f.line = m.line;
        f.name = m.name;
        f.params = m.params;
        f.body = std::move(m.body);
        body.funcs.push_back(std::move(f));
      }
    } else {
      Token t = peek();
      throw std::runtime_error("unexpected token in package body at line " +
                               std::to_string(t.line));
    }
  }
  expect(TokKind::RBrace, "}");

  // `package foo.bar.baz { body }` → nest empty packages with body on the leaf.
  PackageDecl root;
  root.line = line;
  root.name = path[0];
  PackageDecl* leaf = &root;
  for (size_t i = 1; i < path.size(); ++i) {
    auto nested = std::make_unique<PackageDecl>();
    nested->name = path[i];
    leaf->packages.push_back(std::move(nested));
    leaf = leaf->packages.back().get();
  }
  leaf->fields = std::move(body.fields);
  leaf->classes = std::move(body.classes);
  leaf->funcs = std::move(body.funcs);
  leaf->ctor_body = std::move(body.ctor_body);
  leaf->has_ctor = body.has_ctor;
  for (auto& nested : body.packages) {
    leaf->packages.push_back(std::move(nested));
  }
  return root;
}

Method
Parser::parse_method()
{
  const int line = peek().line;
  expect(TokKind::KwFn, "fn");
  Method m;
  m.line = line;
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
  const int line = peek().line;
  expect(TokKind::KwFn, "fn");
  FuncDecl f;
  f.line = line;
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
Parser::parse_my_decls()
{
  // `my a [= e], b [= e], ...`
  expect(TokKind::KwMy, "my");
  std::vector<BlockItem> items;
  for (;;) {
    BlockItem item;
    item.kind = BlockItem::Kind::VarDecl;
    item.line = peek().line;
    item.var_name = expect(TokKind::Ident, "variable name").text;
    if (check(TokKind::Eq)) {
      next();
      item.expr = parse_expr();
    }
    items.push_back(std::move(item));
    if (check(TokKind::Comma)) {
      next();
      continue;
    }
    break;
  }
  return items;
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
      if (check(TokKind::KwMy)) {
        for (auto& item : parse_my_decls()) {
          body.push_back(std::move(item));
        }
      } else {
        body.push_back(parse_block_item());
      }
      while (check(TokKind::Semicolon)) {
        next();
      }
    }
    expect(TokKind::RBrace, "}");
    return body; // may be empty
  }
  if (check(TokKind::KwMy)) {
    return parse_my_decls();
  }
  body.push_back(parse_block_item());
  return body;
}

BlockItem
Parser::parse_block_item()
{
  const int line = peek().line;
  // Nested / scope-local class
  if (check(TokKind::KwClass)) {
    BlockItem item;
    item.kind = BlockItem::Kind::Class;
    item.line = line;
    item.nested_class = std::make_unique<ClassDecl>(parse_class());
    return item;
  }
  // Local: my name [= expr]  (comma-lists handled in parse_method_body)
  if (check(TokKind::KwMy)) {
    auto items = parse_my_decls();
    BlockItem first = std::move(items.front());
    if (items.size() > 1) {
      throw std::runtime_error("multi-variable my only allowed in { } bodies at line " +
                               std::to_string(line));
    }
    return first;
  }
  // Named local function: fn name(...) body  →  my name = fn (...) body
  if (check(TokKind::KwFn)) {
    next(); // consume fn
    if (check(TokKind::Ident)) {
      BlockItem item;
      item.kind = BlockItem::Kind::VarDecl;
      item.line = line;
      item.var_name = next().text;
      auto lam = std::make_unique<Expr>();
      lam->kind = Expr::Kind::Lambda;
      lam->line = line;
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
    lam->line = line;
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
    item.line = line;
    item.expr = std::move(lam);
    return item;
  }
  if (check(TokKind::KwWhile)) {
    BlockItem item;
    item.kind = BlockItem::Kind::Expr;
    item.line = line;
    item.expr = parse_while();
    return item;
  }
  if (check(TokKind::KwIf)) {
    BlockItem item;
    item.kind = BlockItem::Kind::Expr;
    item.line = line;
    item.expr = parse_if();
    return item;
  }
  if (check(TokKind::KwBreak) || check(TokKind::KwContinue)) {
    auto e = std::make_unique<Expr>();
    e->kind = check(TokKind::KwBreak) ? Expr::Kind::Break : Expr::Kind::Continue;
    e->line = line;
    next();
    BlockItem item;
    item.kind = BlockItem::Kind::Expr;
    item.line = line;
    item.expr = std::move(e);
    return item;
  }
  if (check(TokKind::KwReturn)) {
    BlockItem item;
    item.kind = BlockItem::Kind::Expr;
    item.line = line;
    item.expr = parse_return();
    return item;
  }
  BlockItem item;
  item.kind = BlockItem::Kind::Expr;
  item.line = line;
  item.expr = parse_expr();
  return item;
}

ExprPtr
Parser::parse_while()
{
  const int line = peek().line;
  expect(TokKind::KwWhile, "while");
  expect(TokKind::LParen, "(");
  auto e = std::make_unique<Expr>();
  e->kind = Expr::Kind::While;
  e->line = line;
  e->lhs = parse_expr();
  expect(TokKind::RParen, ")");
  e->body = parse_method_body();
  return e;
}

ExprPtr
Parser::parse_if()
{
  const int line = peek().line;
  expect(TokKind::KwIf, "if");
  expect(TokKind::LParen, "(");
  auto e = std::make_unique<Expr>();
  e->kind = Expr::Kind::If;
  e->line = line;
  e->lhs = parse_expr();
  expect(TokKind::RParen, ")");
  e->body = parse_method_body();
  if (check(TokKind::KwElse)) {
    next();
    e->else_body = parse_method_body();
  }
  return e;
}

ExprPtr
Parser::parse_return()
{
  const int line = peek().line;
  expect(TokKind::KwReturn, "return");
  auto e = std::make_unique<Expr>();
  e->kind = Expr::Kind::Return;
  e->line = line;
  // `return` or `return expr` — newline ends a bare return.
  if (check(TokKind::Eof) || check(TokKind::RBrace) || check(TokKind::Semicolon) ||
      peek().after_newline) {
    return e;
  }
  e->rhs = parse_expr();
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
  ExprPtr e = parse_or();
  std::string op;
  if (check(TokKind::Eq)) {
    op = "=";
  } else if (check(TokKind::PlusEq)) {
    op = "+=";
  } else if (check(TokKind::MinusEq)) {
    op = "-=";
  } else if (check(TokKind::StarEq)) {
    op = "*=";
  } else if (check(TokKind::CaretEq)) {
    op = "^=";
  } else if (check(TokKind::AmpEq)) {
    op = "&=";
  } else if (check(TokKind::PipeEq)) {
    op = "|=";
  } else {
    return e;
  }
  next();
  auto a = std::make_unique<Expr>();
  a->kind = Expr::Kind::Assign;
  a->text = op;
  a->lhs = std::move(e);
  a->line = a->lhs->line;
  a->rhs = parse_assign();
  return a;
}

ExprPtr
Parser::parse_or()
{
  ExprPtr e = parse_and();
  while (check(TokKind::PipePipe)) {
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = "||";
    b->lhs = std::move(e);
    b->line = b->lhs->line;
    b->rhs = parse_and();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_and()
{
  // Zef desugars `a && b` to `if (a) b` (else → null).
  ExprPtr e = parse_bitor();
  while (check(TokKind::AmpAmp)) {
    next();
    auto i = std::make_unique<Expr>();
    i->kind = Expr::Kind::If;
    i->lhs = std::move(e);
    i->line = i->lhs->line;
    BlockItem then_item;
    then_item.kind = BlockItem::Kind::Expr;
    then_item.expr = parse_bitor();
    then_item.line = then_item.expr ? then_item.expr->line : i->line;
    i->body.push_back(std::move(then_item));
    e = std::move(i);
  }
  return e;
}

ExprPtr
Parser::parse_bitor()
{
  ExprPtr e = parse_bitxor();
  while (check(TokKind::Pipe)) {
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = "|";
    b->lhs = std::move(e);
    b->line = b->lhs->line;
    b->rhs = parse_bitxor();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_bitxor()
{
  ExprPtr e = parse_bitand();
  while (check(TokKind::Caret)) {
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = "^";
    b->lhs = std::move(e);
    b->line = b->lhs->line;
    b->rhs = parse_bitand();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_bitand()
{
  // Zef: `&` binds looser than `==` (equality-exprs are operands).
  ExprPtr e = parse_equality();
  while (check(TokKind::Amp)) {
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = "&";
    b->lhs = std::move(e);
    b->line = b->lhs->line;
    b->rhs = parse_equality();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_equality()
{
  ExprPtr e = parse_relational();
  for (;;) {
    std::string op;
    if (check(TokKind::EqEq)) {
      op = "==";
    } else if (check(TokKind::BangEq)) {
      op = "!=";
    } else {
      break;
    }
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = op;
    b->lhs = std::move(e);
    b->line = b->lhs->line;
    b->rhs = parse_relational();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_relational()
{
  ExprPtr e = parse_shift();
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
    b->line = b->lhs->line;
    b->rhs = parse_shift();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_shift()
{
  ExprPtr e = parse_add();
  for (;;) {
    std::string op;
    if (check(TokKind::LtLt)) {
      op = "<<";
    } else if (check(TokKind::GtGtGt)) {
      op = ">>>";
    } else if (check(TokKind::GtGt)) {
      op = ">>";
    } else {
      break;
    }
    next();
    auto b = std::make_unique<Expr>();
    b->kind = Expr::Kind::Binary;
    b->text = op;
    b->lhs = std::move(e);
    b->line = b->lhs->line;
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
    b->line = b->lhs->line;
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
    b->line = b->lhs->line;
    b->rhs = parse_unary();
    e = std::move(b);
  }
  return e;
}

ExprPtr
Parser::parse_unary()
{
  // Expression-form `if` (Zef unary): `if (c) then else elze`
  if (check(TokKind::KwIf)) {
    return parse_if();
  }
  if (check(TokKind::Minus) || check(TokKind::Tilde) || check(TokKind::Bang)) {
    const int line = peek().line;
    std::string op = "-";
    if (check(TokKind::Tilde)) {
      op = "~";
    } else if (check(TokKind::Bang)) {
      op = "!";
    }
    next();
    auto u = std::make_unique<Expr>();
    u->kind = Expr::Kind::Unary;
    u->line = line;
    u->text = op;
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
      d->line = d->lhs->line;
      d->text = expect(TokKind::Ident, "member name").text;
      e = std::move(d);
    } else if (check(TokKind::LParen)) {
      auto c = std::make_unique<Expr>();
      c->kind = Expr::Kind::Call;
      c->lhs = std::move(e);
      c->line = c->lhs->line;
      c->args = parse_arg_list();
      e = std::move(c);
    } else if (check(TokKind::LBracket)) {
      next();
      auto ix = std::make_unique<Expr>();
      ix->kind = Expr::Kind::Index;
      ix->lhs = std::move(e);
      ix->line = ix->lhs->line;
      ix->rhs = parse_expr();
      expect(TokKind::RBracket, "]");
      e = std::move(ix);
    } else {
      break;
    }
  }
  return e;
}

ExprPtr
Parser::parse_primary()
{
  const int line = peek().line;
  // `..name` → root-package member (Zef GetRootPackage().name)
  if (check(TokKind::DotDot)) {
    next();
    auto root = std::make_unique<Expr>();
    root->kind = Expr::Kind::RootPackage;
    root->line = line;
    auto d = std::make_unique<Expr>();
    d->kind = Expr::Kind::Dot;
    d->line = line;
    d->lhs = std::move(root);
    d->text = expect(TokKind::Ident, "package name").text;
    return d;
  }
  if (check(TokKind::KwFn)) {
    next();
    auto e = std::make_unique<Expr>();
    e->kind = Expr::Kind::Lambda;
    e->line = line;
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
    e->line = line;
    e->text = next().text;
    return e;
  }
  if (check(TokKind::String)) {
    auto e = std::make_unique<Expr>();
    e->kind = Expr::Kind::String;
    e->line = line;
    e->text = next().text;
    return e;
  }
  if (check(TokKind::Number)) {
    auto e = std::make_unique<Expr>();
    e->kind = Expr::Kind::Number;
    e->line = line;
    e->text = next().text;
    return e;
  }
  if (check(TokKind::LParen)) {
    next();
    ExprPtr e = parse_expr();
    expect(TokKind::RParen, ")");
    return e;
  }
  if (check(TokKind::LBracket)) {
    next();
    auto e = std::make_unique<Expr>();
    e->kind = Expr::Kind::ArrayLit;
    e->line = line;
    if (!check(TokKind::RBracket)) {
      e->args.push_back(parse_expr());
      while (check(TokKind::Comma)) {
        next();
        e->args.push_back(parse_expr());
      }
    }
    expect(TokKind::RBracket, "]");
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
