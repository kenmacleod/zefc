#include "codegen.hpp"

#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace zefc {
namespace compiler {
namespace {

std::string
mangle_escape(const std::string& s)
{
  std::string o;
  for (char c : s) {
    if (c == '\\') {
      o += "\\\\";
    } else if (c == '"') {
      o += "\\\"";
    } else if (c == '\n') {
      o += "\\n";
    } else {
      o.push_back(c);
    }
  }
  return o;
}

std::string
mangle_method(const std::string& name, size_t arity)
{
  std::string m = name + "_";
  for (size_t i = 0; i < arity; ++i) {
    m.push_back('o');
  }
  if (arity == 0) {
    m.push_back('o');
  }
  return m;
}

std::string
mangle_getter(const std::string& field)
{
  return field + "_o";
}

std::string
mangle_setter(const std::string& field)
{
  return "set_" + field + "_o";
}

std::string
sel_expr(const std::string& mangled)
{
  if (mangled == "toString_o") {
    return "ZEFC_SEL_toString_o";
  }
  if (mangled == "add_o") {
    return "ZEFC_SEL_add_o";
  }
  if (mangled == "sub_o") {
    return "ZEFC_SEL_sub_o";
  }
  if (mangled == "mul_o") {
    return "ZEFC_SEL_mul_o";
  }
  if (mangled == "div_o") {
    return "ZEFC_SEL_div_o";
  }
  if (mangled == "push_o") {
    return "ZEFC_SEL_push_o";
  }
  if (mangled == "GET_i") {
    return "ZEFC_SEL_GET_i";
  }
  if (mangled == "PUT_i") {
    return "ZEFC_SEL_PUT_i";
  }
  if (mangled == "mul_PUT_i") {
    return "ZEFC_SEL_mul_PUT_i";
  }
  return "ZEFC_SITE(\"" + mangled + "\")";
}

struct ClassInfo {
  const ClassDecl* decl = nullptr;
  std::unordered_set<std::string> field_names;
  std::unordered_set<std::string> param_names;
  std::unordered_set<std::string> method_names; // zero-arg instance methods (bare Ident call)
  bool has_static = false;
  bool has_static_call0 = false; // static fn call with 0 params
};

struct FuncInfo {
  const FuncDecl* decl = nullptr;
  size_t arity = 0;
};

struct Ctx {
  std::ostringstream out;
  std::ostringstream prelude; // closure types/methods at namespace scope
  std::unordered_map<std::string, ClassInfo> classes;
  std::unordered_map<std::string, FuncInfo> functions;
  std::vector<std::string> toplevel_vars;
  const ClassInfo* current_class = nullptr;
  std::vector<std::string> env_params;
  int tmp = 0;
  int closure_id = 0;

  std::string fresh(const char* prefix)
  {
    return std::string(prefix) + std::to_string(tmp++);
  }
};

void emit_expr(Ctx& ctx, const Expr& e, const std::string& dst);
void emit_expr_top(Ctx& ctx, const Expr& e, const std::string& dst);
void emit_body(Ctx& ctx, const std::vector<BlockItem>& body, const std::string& result_dst,
               bool ctor_return_self);
void emit_block_item(Ctx& ctx, const BlockItem& item, const std::string* result_dst);

bool
is_field(const Ctx& ctx, const std::string& name)
{
  if (!ctx.current_class) {
    return false;
  }
  if (ctx.current_class->param_names.count(name)) {
    return false;
  }
  return ctx.current_class->field_names.count(name);
}

void
emit_send(Ctx& ctx, const std::string& recv, const std::string& mangled,
          const std::vector<std::string>& args, const std::string& dst)
{
  const std::string sel = sel_expr(mangled);
  if (args.empty()) {
    ctx.out << "  " << dst << " = ZEFC_SEND0(" << recv << ", " << sel << ");\n";
  } else if (args.size() == 1) {
    ctx.out << "  " << dst << " = ZEFC_SEND1(" << recv << ", " << sel << ", " << args[0]
            << ");\n";
  } else if (args.size() == 2) {
    ctx.out << "  " << dst << " = ZEFC_SEND2(" << recv << ", " << sel << ", " << args[0]
            << ", " << args[1] << ");\n";
  } else if (args.size() == 3) {
    ctx.out << "  " << dst << " = ZEFC_SEND3(" << recv << ", " << sel << ", " << args[0]
            << ", " << args[1] << ", " << args[2] << ");\n";
  } else if (args.size() == 4) {
    ctx.out << "  " << dst << " = ZEFC_SEND4(" << recv << ", " << sel << ", " << args[0]
            << ", " << args[1] << ", " << args[2] << ", " << args[3] << ");\n";
  } else {
    throw std::runtime_error("send arity >4 not supported yet");
  }
}

void
collect_free(const Expr& e, const std::unordered_set<std::string>& locals,
             const std::vector<std::string>& env, std::vector<std::string>& captures)
{
  auto add_cap = [&](const std::string& name) {
    if (locals.count(name)) {
      return;
    }
    bool in_env = false;
    for (const std::string& p : env) {
      if (p == name) {
        in_env = true;
        break;
      }
    }
    if (!in_env) {
      return;
    }
    for (const std::string& c : captures) {
      if (c == name) {
        return;
      }
    }
    captures.push_back(name);
  };

  switch (e.kind) {
  case Expr::Kind::Ident:
    add_cap(e.text);
    break;
  case Expr::Kind::Dot:
    if (e.lhs) {
      collect_free(*e.lhs, locals, env, captures);
    }
    break;
  case Expr::Kind::Index:
    if (e.lhs) {
      collect_free(*e.lhs, locals, env, captures);
    }
    if (e.rhs) {
      collect_free(*e.rhs, locals, env, captures);
    }
    break;
  case Expr::Kind::ArrayLit:
    for (const auto& a : e.args) {
      collect_free(*a, locals, env, captures);
    }
    break;
  case Expr::Kind::Binary:
  case Expr::Kind::Assign:
  case Expr::Kind::Unary:
    if (e.lhs) {
      collect_free(*e.lhs, locals, env, captures);
    }
    if (e.rhs) {
      collect_free(*e.rhs, locals, env, captures);
    }
    break;
  case Expr::Kind::Call:
    if (e.lhs) {
      collect_free(*e.lhs, locals, env, captures);
    }
    for (const auto& a : e.args) {
      collect_free(*a, locals, env, captures);
    }
    break;
  case Expr::Kind::While:
  case Expr::Kind::If:
    if (e.lhs) {
      collect_free(*e.lhs, locals, env, captures);
    }
    for (const BlockItem& item : e.body) {
      if (item.kind == BlockItem::Kind::VarDecl) {
        if (item.expr) {
          collect_free(*item.expr, locals, env, captures);
        }
      } else if (item.expr) {
        collect_free(*item.expr, locals, env, captures);
      }
    }
    for (const BlockItem& item : e.else_body) {
      if (item.kind == BlockItem::Kind::VarDecl) {
        if (item.expr) {
          collect_free(*item.expr, locals, env, captures);
        }
      } else if (item.expr) {
        collect_free(*item.expr, locals, env, captures);
      }
    }
    break;
  case Expr::Kind::Return:
    if (e.rhs) {
      collect_free(*e.rhs, locals, env, captures);
    }
    break;
  case Expr::Kind::Break:
  case Expr::Kind::Continue:
    break;
  case Expr::Kind::Lambda: {
    // Nested lambda: free vars relative to nested_env = env ∪ locals.
    // Enclosing captures only those nested frees that are in env (not locals).
    std::unordered_set<std::string> nested_locals;
    for (const std::string& p : e.params) {
      nested_locals.insert(p);
    }
    std::vector<std::string> nested_env = env;
    for (const std::string& l : locals) {
      nested_env.push_back(l);
    }
    std::vector<std::string> nested_caps;
    for (const BlockItem& item : e.body) {
      if (item.kind == BlockItem::Kind::VarDecl) {
        if (item.expr) {
          collect_free(*item.expr, nested_locals, nested_env, nested_caps);
        }
        nested_locals.insert(item.var_name);
      } else if (item.expr) {
        collect_free(*item.expr, nested_locals, nested_env, nested_caps);
      }
    }
    for (const std::string& c : nested_caps) {
      add_cap(c);
    }
    break;
  }
  default:
    break;
  }
}

void
emit_lambda(Ctx& ctx, const Expr& e, const std::string& dst)
{
  std::unordered_set<std::string> locals;
  for (const std::string& p : e.params) {
    locals.insert(p);
  }
  std::vector<std::string> captures;
  {
    std::unordered_set<std::string> scan_locals = locals;
    for (const BlockItem& item : e.body) {
      if (item.kind == BlockItem::Kind::VarDecl) {
        if (item.expr) {
          collect_free(*item.expr, scan_locals, ctx.env_params, captures);
        }
        scan_locals.insert(item.var_name);
      } else if (item.expr) {
        collect_free(*item.expr, scan_locals, ctx.env_params, captures);
      }
    }
  }

  const int id = ctx.closure_id++;
  const std::string cname = "Closure" + std::to_string(id);

  ctx.prelude << "struct " << cname << "_ {\n  IsaPtr isa_;\n";
  for (const std::string& cap : captures) {
    ctx.prelude << "  id " << cap << ";\n";
  }
  ctx.prelude << "};\n";
  ctx.prelude << "static VTable* " << cname << "_vtable = nullptr;\n";

  // Build call method into a temporary Ctx sharing class env for body.
  std::ostringstream body_out;
  Ctx body_ctx;
  body_ctx.classes = ctx.classes;
  body_ctx.functions = ctx.functions;
  body_ctx.tmp = ctx.tmp;
  body_ctx.closure_id = ctx.closure_id;
  ClassDecl fake;
  fake.name = cname;
  for (const std::string& cap : captures) {
    Field f;
    f.name = cap;
    fake.fields.push_back(std::move(f));
  }
  ClassInfo fake_info;
  fake_info.decl = &fake;
  for (const std::string& cap : captures) {
    fake_info.field_names.insert(cap);
  }
  for (const std::string& p : e.params) {
    fake_info.param_names.insert(p);
  }
  body_ctx.current_class = &fake_info;
  // Nested lambdas may capture outer params + already-captured env.
  body_ctx.env_params = ctx.env_params;
  for (const std::string& p : e.params) {
    body_ctx.env_params.push_back(p);
  }
  body_ctx.out << "static id\n" << cname << "__call_o(id self, int selector";
  for (size_t i = 0; i < e.params.size(); ++i) {
    body_ctx.out << ", id " << e.params[i];
  }
  body_ctx.out << ")\n{\n  (void)selector;\n";
  const std::string tmp = body_ctx.fresh("t");
  body_ctx.out << "  id " << tmp << ";\n";
  emit_body(body_ctx, e.body, tmp, false);
  body_ctx.out << "}\n\n";
  ctx.tmp = body_ctx.tmp;
  ctx.closure_id = body_ctx.closure_id;
  // Nested closures first, then this call method (call body may reference them).
  ctx.prelude << body_ctx.prelude.str();
  ctx.prelude << body_ctx.out.str();

  ctx.out << "  if (!" << cname << "_vtable) {\n";
  ctx.out << "    " << cname << "_vtable = vtable_create();\n";
  ctx.out << "    vtable_set(" << cname << "_vtable, selector_intern(\"call_o\"), " << cname
          << "__call_o);\n";
  ctx.out << "    vtable_set(" << cname << "_vtable, selector_intern(\"add_o\"), " << cname
          << "__call_o);\n";
  ctx.out << "  }\n";
  ctx.out << "  {\n";
  ctx.out << "    " << cname << "_* _c = alloc<" << cname << "_>();\n";
  ctx.out << "    zefc_set_isa(_c, " << cname << "_vtable);\n";
  for (const std::string& cap : captures) {
    if (is_field(ctx, cap)) {
      ctx.out << "    _c->" << cap << " = body<" << ctx.current_class->decl->name << "_>(self)->"
              << cap << ";\n";
    } else {
      ctx.out << "    _c->" << cap << " = " << cap << ";\n";
    }
  }
  ctx.out << "    " << dst << " = as_id(_c);\n";
  ctx.out << "  }\n";
}

void
emit_expr(Ctx& ctx, const Expr& e, const std::string& dst)
{
  switch (e.kind) {
  case Expr::Kind::Ident: {
    if (e.text == "super") {
      if (!ctx.current_class || ctx.current_class->decl->parent.empty()) {
        throw std::runtime_error("super outside subclass");
      }
      // Bare `super` → parent zero-arg init on same object.
      const std::string& parent = ctx.current_class->decl->parent;
      ctx.out << "  " << dst << " = " << parent << "__init(self, 0);\n";
      return;
    }
    if (is_field(ctx, e.text)) {
      const std::string& cls = ctx.current_class->decl->name;
      ctx.out << "  " << dst << " = body<" << cls << "_>(self)->" << e.text << ";\n";
    } else if (ctx.current_class && ctx.current_class->method_names.count(e.text) &&
               !ctx.current_class->param_names.count(e.text)) {
      // Bare method name → self.method (e.g. `fn thingy stuff`)
      emit_send(ctx, "self", mangle_method(e.text, 0), {}, dst);
    } else if (ctx.functions.count(e.text) && ctx.functions[e.text].arity == 0) {
      // Bare zero-arg function name → call (println(foo), property getters).
      ctx.out << "  " << dst << " = fn_" << e.text << "();\n";
    } else {
      ctx.out << "  " << dst << " = " << e.text << ";\n";
    }
    return;
  }
  case Expr::Kind::Lambda:
    emit_lambda(ctx, e, dst);
    return;
  case Expr::Kind::ArrayLit: {
    ctx.out << "  " << dst << " = Array__new();\n";
    for (const auto& a : e.args) {
      const std::string el = ctx.fresh("t");
      ctx.out << "  id " << el << ";\n";
      emit_expr(ctx, *a, el);
      ctx.out << "  (void)ZEFC_SEND1(" << dst << ", ZEFC_SEL_push_o, " << el << ");\n";
    }
    return;
  }
  case Expr::Kind::Index: {
    const std::string recv = ctx.fresh("t");
    const std::string ix = ctx.fresh("t");
    ctx.out << "  id " << recv << ";\n  id " << ix << ";\n";
    emit_expr(ctx, *e.lhs, recv);
    emit_expr(ctx, *e.rhs, ix);
    ctx.out << "  " << dst << " = ZEFC_SEND1(" << recv << ", ZEFC_SEL_GET_i, " << ix << ");\n";
    return;
  }
  case Expr::Kind::String:
    ctx.out << "  " << dst << " = String__from_utf8(\"" << mangle_escape(e.text) << "\");\n";
    return;
  case Expr::Kind::Number:
    if (e.text.size() >= 2 && e.text[0] == '0' && (e.text[1] == 'x' || e.text[1] == 'X')) {
      ctx.out << "  " << dst << " = Int__from_i64(static_cast<long long>(" << e.text
              << "ULL));\n";
    } else {
      ctx.out << "  " << dst << " = Int__from_i64(" << e.text << ");\n";
    }
    return;
  case Expr::Kind::Dot: {
    // super.method → direct superclass method call
    if (e.lhs && e.lhs->kind == Expr::Kind::Ident && e.lhs->text == "super") {
      if (!ctx.current_class || ctx.current_class->decl->parent.empty()) {
        throw std::runtime_error("super.method outside subclass");
      }
      const std::string& parent = ctx.current_class->decl->parent;
      const std::string mangled = mangle_method(e.text, 0);
      ctx.out << "  " << dst << " = " << parent << "__" << mangled << "(self, "
              << sel_expr(mangled) << ");\n";
      return;
    }
    // ClassName.field → static
    if (e.lhs && e.lhs->kind == Expr::Kind::Ident && ctx.classes.count(e.lhs->text)) {
      const std::string& cls = e.lhs->text;
      ctx.out << "  " << dst << " = g_" << cls << "Class." << e.text << ";\n";
      return;
    }
    const std::string recv = ctx.fresh("t");
    ctx.out << "  id " << recv << ";\n";
    emit_expr(ctx, *e.lhs, recv);
    emit_send(ctx, recv, mangle_method(e.text, 0), {}, dst);
    return;
  }
  case Expr::Kind::Binary: {
    const std::string a = ctx.fresh("t");
    const std::string b = ctx.fresh("t");
    ctx.out << "  id " << a << ";\n  id " << b << ";\n";
    // Desugar > / >= to swapped < / <= (Zef semantics).
    std::string op = e.text;
    const Expr* left = e.lhs.get();
    const Expr* right = e.rhs.get();
    if (op == ">") {
      op = "<";
      left = e.rhs.get();
      right = e.lhs.get();
    } else if (op == ">=") {
      op = "<=";
      left = e.rhs.get();
      right = e.lhs.get();
    }
    emit_expr(ctx, *left, a);
    emit_expr(ctx, *right, b);
    if (op == "+") {
      ctx.out << "  " << dst << " = ZEFC_SEND1(" << a << ", ZEFC_SEL_add_o, " << b << ");\n";
    } else if (op == "-") {
      ctx.out << "  " << dst << " = ZEFC_SEND1(" << a << ", ZEFC_SEL_sub_o, " << b << ");\n";
    } else if (op == "*") {
      ctx.out << "  " << dst << " = ZEFC_SEND1(" << a << ", ZEFC_SEL_mul_o, " << b << ");\n";
    } else if (op == "/") {
      ctx.out << "  " << dst << " = ZEFC_SEND1(" << a << ", ZEFC_SEL_div_o, " << b << ");\n";
    } else if (op == "%") {
      ctx.out << "  " << dst << " = ZEFC_SEND1(" << a << ", ZEFC_SITE(\"mod_o\"), " << b
              << ");\n";
    } else if (op == "==") {
      ctx.out << "  " << dst << " = Int__from_i64((" << a << " == " << b << ") ? 1 : 0);\n";
    } else if (op == "<") {
      ctx.out << "  " << dst << " = Int__from_i64((Int__to_i64(" << a << ") < Int__to_i64(" << b
              << ")) ? 1 : 0);\n";
    } else if (op == "<=") {
      ctx.out << "  " << dst << " = Int__from_i64((Int__to_i64(" << a << ") <= Int__to_i64(" << b
              << ")) ? 1 : 0);\n";
    } else if (op == "&") {
      ctx.out << "  " << dst << " = Int__from_i64(Int__to_i64(" << a << ") & Int__to_i64(" << b
              << "));\n";
    } else {
      throw std::runtime_error("unsupported binary op: " + e.text);
    }
    return;
  }
  case Expr::Kind::While: {
    const std::string cond = ctx.fresh("t");
    ctx.out << "  while (true) {\n";
    ctx.out << "  id " << cond << ";\n";
    emit_expr(ctx, *e.lhs, cond);
    ctx.out << "  if (!Int__to_i64(" << cond << ")) {\n    break;\n  }\n";
    for (const BlockItem& item : e.body) {
      emit_block_item(ctx, item, nullptr);
    }
    ctx.out << "  }\n";
    ctx.out << "  " << dst << " = null_id();\n";
    return;
  }
  case Expr::Kind::If: {
    const std::string cond = ctx.fresh("t");
    ctx.out << "  id " << cond << ";\n";
    emit_expr(ctx, *e.lhs, cond);
    ctx.out << "  if (Int__to_i64(" << cond << ")) {\n";
    for (const BlockItem& item : e.body) {
      emit_block_item(ctx, item, nullptr);
    }
    ctx.out << "  }";
    if (!e.else_body.empty()) {
      ctx.out << " else {\n";
      for (const BlockItem& item : e.else_body) {
        emit_block_item(ctx, item, nullptr);
      }
      ctx.out << "  }";
    }
    ctx.out << "\n";
    ctx.out << "  " << dst << " = null_id();\n";
    return;
  }
  case Expr::Kind::Break:
    ctx.out << "  break;\n";
    ctx.out << "  " << dst << " = null_id();\n";
    return;
  case Expr::Kind::Continue:
    ctx.out << "  continue;\n";
    ctx.out << "  " << dst << " = null_id();\n";
    return;
  case Expr::Kind::Return: {
    if (e.rhs) {
      const std::string v = ctx.fresh("t");
      ctx.out << "  id " << v << ";\n";
      emit_expr(ctx, *e.rhs, v);
      ctx.out << "  return " << v << ";\n";
    } else {
      ctx.out << "  return null_id();\n";
    }
    ctx.out << "  " << dst << " = null_id();\n";
    return;
  }
  case Expr::Kind::Unary: {
    if (e.text != "-") {
      throw std::runtime_error("unsupported unary op: " + e.text);
    }
    const std::string a = ctx.fresh("t");
    const std::string z = ctx.fresh("t");
    ctx.out << "  id " << a << ";\n  id " << z << ";\n";
    emit_expr(ctx, *e.rhs, a);
    ctx.out << "  " << z << " = Int__from_i64(0);\n";
    ctx.out << "  " << dst << " = ZEFC_SEND1(" << z << ", ZEFC_SEL_sub_o, " << a << ");\n";
    return;
  }
  case Expr::Kind::Assign: {
    const bool plus_eq = (e.text == "+=");
    const bool star_eq = (e.text == "*=");
    const std::string rhs = ctx.fresh("t");
    ctx.out << "  id " << rhs << ";\n";

    // a[i] *= v → mul_PUT_i
    if (star_eq && e.lhs && e.lhs->kind == Expr::Kind::Index) {
      const std::string recv = ctx.fresh("t");
      const std::string ix = ctx.fresh("t");
      ctx.out << "  id " << recv << ";\n  id " << ix << ";\n";
      emit_expr(ctx, *e.lhs->lhs, recv);
      emit_expr(ctx, *e.lhs->rhs, ix);
      emit_expr(ctx, *e.rhs, rhs);
      ctx.out << "  " << dst << " = ZEFC_SEND2(" << recv << ", ZEFC_SEL_mul_PUT_i, " << ix
              << ", " << rhs << ");\n";
      return;
    }
    // a[i] = v → PUT_i
    if (!plus_eq && !star_eq && e.lhs && e.lhs->kind == Expr::Kind::Index) {
      const std::string recv = ctx.fresh("t");
      const std::string ix = ctx.fresh("t");
      ctx.out << "  id " << recv << ";\n  id " << ix << ";\n";
      emit_expr(ctx, *e.lhs->lhs, recv);
      emit_expr(ctx, *e.lhs->rhs, ix);
      emit_expr(ctx, *e.rhs, rhs);
      ctx.out << "  " << dst << " = ZEFC_SEND2(" << recv << ", ZEFC_SEL_PUT_i, " << ix << ", "
              << rhs << ");\n";
      return;
    }

    if (plus_eq) {
      // lhs += rhs  →  lhs = lhs + rhs
      const std::string cur = ctx.fresh("t");
      const std::string addend = ctx.fresh("t");
      ctx.out << "  id " << cur << ";\n  id " << addend << ";\n";
      if (e.lhs && e.lhs->kind == Expr::Kind::Ident && is_field(ctx, e.lhs->text)) {
        const std::string& cls = ctx.current_class->decl->name;
        ctx.out << "  " << cur << " = body<" << cls << "_>(self)->" << e.lhs->text << ";\n";
      } else if (e.lhs && e.lhs->kind == Expr::Kind::Ident) {
        ctx.out << "  " << cur << " = " << e.lhs->text << ";\n";
      } else if (e.lhs && e.lhs->kind == Expr::Kind::Dot) {
        emit_expr(ctx, *e.lhs, cur);
      } else if (e.lhs && e.lhs->kind == Expr::Kind::Index) {
        emit_expr(ctx, *e.lhs, cur);
      } else {
        throw std::runtime_error("+= target must be an identifier, field, or index");
      }
      emit_expr(ctx, *e.rhs, addend);
      ctx.out << "  " << rhs << " = ZEFC_SEND1(" << cur << ", ZEFC_SEL_add_o, " << addend
              << ");\n";
    } else if (star_eq) {
      const std::string cur = ctx.fresh("t");
      const std::string factor = ctx.fresh("t");
      ctx.out << "  id " << cur << ";\n  id " << factor << ";\n";
      if (e.lhs && e.lhs->kind == Expr::Kind::Ident && is_field(ctx, e.lhs->text)) {
        const std::string& cls = ctx.current_class->decl->name;
        ctx.out << "  " << cur << " = body<" << cls << "_>(self)->" << e.lhs->text << ";\n";
      } else if (e.lhs && e.lhs->kind == Expr::Kind::Ident) {
        ctx.out << "  " << cur << " = " << e.lhs->text << ";\n";
      } else if (e.lhs && e.lhs->kind == Expr::Kind::Dot) {
        emit_expr(ctx, *e.lhs, cur);
      } else {
        throw std::runtime_error("*= target must be an identifier or field");
      }
      emit_expr(ctx, *e.rhs, factor);
      ctx.out << "  " << rhs << " = ZEFC_SEND1(" << cur << ", ZEFC_SEL_mul_o, " << factor
              << ");\n";
    } else {
      emit_expr(ctx, *e.rhs, rhs);
    }
    if (e.lhs && e.lhs->kind == Expr::Kind::Dot) {
      // ClassName.field = rhs (static)
      if (e.lhs->lhs && e.lhs->lhs->kind == Expr::Kind::Ident &&
          ctx.classes.count(e.lhs->lhs->text)) {
        ctx.out << "  g_" << e.lhs->lhs->text << "Class." << e.lhs->text << " = " << rhs
                << ";\n";
        ctx.out << "  " << dst << " = " << rhs << ";\n";
        return;
      }
      // obj.field = rhs → setter send
      const std::string recv = ctx.fresh("t");
      ctx.out << "  id " << recv << ";\n";
      emit_expr(ctx, *e.lhs->lhs, recv);
      emit_send(ctx, recv, mangle_setter(e.lhs->text), {rhs}, dst);
      return;
    }
    if (e.lhs && e.lhs->kind == Expr::Kind::Index) {
      const std::string recv = ctx.fresh("t");
      const std::string ix = ctx.fresh("t");
      ctx.out << "  id " << recv << ";\n  id " << ix << ";\n";
      emit_expr(ctx, *e.lhs->lhs, recv);
      emit_expr(ctx, *e.lhs->rhs, ix);
      ctx.out << "  " << dst << " = ZEFC_SEND2(" << recv << ", ZEFC_SEL_PUT_i, " << ix << ", "
              << rhs << ");\n";
      return;
    }
    if (!e.lhs || e.lhs->kind != Expr::Kind::Ident) {
      throw std::runtime_error("assignment target must be an identifier or field");
    }
    const std::string& name = e.lhs->text;
    if (is_field(ctx, name)) {
      const std::string& cls = ctx.current_class->decl->name;
      ctx.out << "  body<" << cls << "_>(self)->" << name << " = " << rhs << ";\n";
      ctx.out << "  " << dst << " = " << rhs << ";\n";
    } else if (ctx.functions.count("set_" + name)) {
      // Top-level property: x = v → set_x(v)
      ctx.out << "  " << dst << " = fn_set_" << name << "(" << rhs << ");\n";
    } else {
      ctx.out << "  " << name << " = " << rhs << ";\n";
      ctx.out << "  " << dst << " = " << rhs << ";\n";
    }
    return;
  }
  case Expr::Kind::Call: {
    // obj.method(args) — including nested receivers like a.b(1).c(2)
    if (e.lhs && e.lhs->kind == Expr::Kind::Dot) {
      std::vector<std::string> arg_tmps;
      for (size_t i = 0; i < e.args.size(); ++i) {
        arg_tmps.push_back(ctx.fresh("t"));
        ctx.out << "  id " << arg_tmps.back() << ";\n";
        emit_expr(ctx, *e.args[i], arg_tmps.back());
      }
      if (e.lhs->lhs && e.lhs->lhs->kind == Expr::Kind::Ident && e.lhs->lhs->text == "super") {
        if (!ctx.current_class || ctx.current_class->decl->parent.empty()) {
          throw std::runtime_error("super.method outside subclass");
        }
        const std::string& parent = ctx.current_class->decl->parent;
        const std::string mangled = mangle_method(e.lhs->text, e.args.size());
        ctx.out << "  " << dst << " = " << parent << "__" << mangled << "(self, "
                << sel_expr(mangled);
        for (const std::string& a : arg_tmps) {
          ctx.out << ", " << a;
        }
        ctx.out << ");\n";
        return;
      }
      const std::string recv = ctx.fresh("t");
      ctx.out << "  id " << recv << ";\n";
      emit_expr(ctx, *e.lhs->lhs, recv);
      emit_send(ctx, recv, mangle_method(e.lhs->text, e.args.size()), arg_tmps, dst);
      return;
    }
    std::vector<std::string> arg_tmps;
    for (size_t i = 0; i < e.args.size(); ++i) {
      arg_tmps.push_back(ctx.fresh("t"));
      ctx.out << "  id " << arg_tmps.back() << ";\n";
      emit_expr(ctx, *e.args[i], arg_tmps.back());
    }
    if (e.lhs && e.lhs->kind == Expr::Kind::Ident) {
      const std::string& callee = e.lhs->text;
      if (callee == "super") {
        if (!ctx.current_class || ctx.current_class->decl->parent.empty()) {
          throw std::runtime_error("super(...) outside subclass");
        }
        const std::string& parent = ctx.current_class->decl->parent;
        ctx.out << "  " << dst << " = " << parent << "__init(self, 0";
        for (const std::string& a : arg_tmps) {
          ctx.out << ", " << a;
        }
        ctx.out << ");\n";
        return;
      }
      if (callee == "println" || callee == "print") {
        if (arg_tmps.empty()) {
          throw std::runtime_error(callee + " expects at least 1 argument");
        }
        if (arg_tmps.size() == 1) {
          ctx.out << "  " << callee << "(" << arg_tmps[0] << ");\n";
        } else {
          const std::string acc = ctx.fresh("t");
          ctx.out << "  id " << acc << " = String__from_utf8(\"\");\n";
          for (const std::string& a : arg_tmps) {
            const std::string s = ctx.fresh("t");
            ctx.out << "  id " << s << " = ZEFC_SEND0(" << a << ", ZEFC_SEL_toString_o);\n";
            ctx.out << "  " << acc << " = ZEFC_SEND1(" << acc << ", ZEFC_SEL_add_o, " << s
                    << ");\n";
          }
          ctx.out << "  " << callee << "(" << acc << ");\n";
        }
        ctx.out << "  " << dst << " = null_id();\n";
        return;
      }
      if (callee == "String") {
        if (arg_tmps.empty()) {
          ctx.out << "  " << dst << " = String__from_utf8(\"\");\n";
        } else {
          ctx.out << "  " << dst << " = ZEFC_SEND0(" << arg_tmps[0] << ", ZEFC_SEL_toString_o);\n";
          for (size_t i = 1; i < arg_tmps.size(); ++i) {
            const std::string s = ctx.fresh("t");
            ctx.out << "  id " << s << " = ZEFC_SEND0(" << arg_tmps[i]
                    << ", ZEFC_SEL_toString_o);\n";
            ctx.out << "  " << dst << " = ZEFC_SEND1(" << dst << ", ZEFC_SEL_add_o, " << s
                    << ");\n";
          }
        }
        return;
      }
      if (ctx.classes.count(callee)) {
        const ClassInfo& ci = ctx.classes[callee];
        // Class() with static call and no ctor args → invoke class.call (Zef staticcall).
        if (arg_tmps.empty() && ci.has_static_call0) {
          ctx.out << "  " << dst << " = " << callee << "__call_o(as_id(&g_" << callee
                  << "Class), " << sel_expr("call_o") << ");\n";
        } else {
          ctx.out << "  " << dst << " = " << callee << "__new(null_id(), 0";
          for (const std::string& a : arg_tmps) {
            ctx.out << ", " << a;
          }
          ctx.out << ");\n";
        }
        return;
      }
      if (ctx.functions.count(callee)) {
        ctx.out << "  " << dst << " = fn_" << callee << "(";
        for (size_t i = 0; i < arg_tmps.size(); ++i) {
          if (i) {
            ctx.out << ", ";
          }
          ctx.out << arg_tmps[i];
        }
        ctx.out << ");\n";
        return;
      }
    }
    // Callable apply: recv(args) → call_o / add_o
    const std::string recv = ctx.fresh("t");
    ctx.out << "  id " << recv << ";\n";
    emit_expr(ctx, *e.lhs, recv);
    emit_send(ctx, recv, "call_o", arg_tmps, dst);
    return;
  }
  }
  throw std::runtime_error("unhandled expression kind");
}

void
emit_expr_top(Ctx& ctx, const Expr& e, const std::string& dst)
{
  emit_expr(ctx, e, dst);
}

void
emit_block_item(Ctx& ctx, const BlockItem& item, const std::string* result_dst)
{
  if (item.kind == BlockItem::Kind::VarDecl) {
    ctx.out << "  id " << item.var_name;
    if (item.expr) {
      ctx.out << ";\n";
      emit_expr_top(ctx, *item.expr, item.var_name);
    } else {
      ctx.out << " = null_id();\n";
    }
    ctx.env_params.push_back(item.var_name);
    if (result_dst) {
      ctx.out << "  " << *result_dst << " = " << item.var_name << ";\n";
    }
    return;
  }
  if (!item.expr) {
    if (result_dst) {
      ctx.out << "  " << *result_dst << " = null_id();\n";
    }
    return;
  }
  if (result_dst) {
    emit_expr_top(ctx, *item.expr, *result_dst);
  } else {
    const std::string t = ctx.fresh("t");
    ctx.out << "  id " << t << ";\n";
    emit_expr_top(ctx, *item.expr, t);
  }
}

void
emit_body(Ctx& ctx, const std::vector<BlockItem>& body, const std::string& result_dst,
          bool ctor_return_self)
{
  if (body.empty()) {
    if (ctor_return_self) {
      ctx.out << "  return self;\n";
    } else {
      ctx.out << "  " << result_dst << " = null_id();\n";
      ctx.out << "  return " << result_dst << ";\n";
    }
    return;
  }
  for (size_t i = 0; i + 1 < body.size(); ++i) {
    emit_block_item(ctx, body[i], nullptr);
  }
  emit_block_item(ctx, body.back(), &result_dst);
  if (ctor_return_self) {
    ctx.out << "  return self;\n";
  } else {
    ctx.out << "  return " << result_dst << ";\n";
  }
}

void
collect_classes(Ctx& ctx, const Program& program)
{
  for (const Stmt& s : program.stmts) {
    if (s.kind == Stmt::Kind::Func) {
      FuncInfo fi;
      fi.decl = &s.func_decl;
      fi.arity = s.func_decl.params.size();
      ctx.functions[s.func_decl.name] = fi;
      continue;
    }
    if (s.kind != Stmt::Kind::Class) {
      continue;
    }
    ClassInfo info;
    info.decl = &s.class_decl;
    for (const Field& f : s.class_decl.fields) {
      if (!f.is_static) {
        info.field_names.insert(f.name);
      }
      if (f.is_static) {
        info.has_static = true;
      }
    }
    for (const Method& m : s.class_decl.methods) {
      if (m.is_static) {
        info.has_static = true;
        if (m.name == "call" && m.params.empty()) {
          info.has_static_call0 = true;
        }
      } else if (!m.name.empty() && m.params.empty()) {
        info.method_names.insert(m.name);
      }
    }
    // Inherit parent instance field names for is_field in methods
    if (!s.class_decl.parent.empty()) {
      // parent may appear later; field merge deferred in emit if needed
    }
    ctx.classes[s.class_decl.name] = std::move(info);
  }
  // Second pass: merge parent field names
  for (auto& kv : ctx.classes) {
    std::string p = kv.second.decl->parent;
    while (!p.empty()) {
      auto it = ctx.classes.find(p);
      if (it == ctx.classes.end()) {
        break;
      }
      for (const Field& f : it->second.decl->fields) {
        if (!f.is_static) {
          kv.second.field_names.insert(f.name);
        }
      }
      p = it->second.decl->parent;
    }
  }
}

void
emit_accessor_methods(Ctx& ctx, const ClassDecl& c)
{
  const std::string& name = c.name;
  for (const Field& f : c.fields) {
    if (f.is_static) {
      continue;
    }
    if (f.readable || f.accessible) {
      const std::string g = mangle_getter(f.name);
      ctx.out << "static id\n" << name << "__" << g << "(id self, int selector)\n{\n";
      ctx.out << "  (void)selector;\n";
      ctx.out << "  return body<" << name << "_>(self)->" << f.name << ";\n}\n\n";
    }
    if (f.accessible) {
      const std::string s = mangle_setter(f.name);
      ctx.out << "static id\n" << name << "__" << s << "(id self, int selector, id v)\n{\n";
      ctx.out << "  (void)selector;\n";
      ctx.out << "  body<" << name << "_>(self)->" << f.name << " = v;\n";
      ctx.out << "  return null_id();\n}\n\n";
    }
  }
}

void
emit_class(Ctx& ctx, const ClassDecl& c)
{
  const std::string& name = c.name;
  ClassInfo* info = &ctx.classes[name];

  ctx.out << "struct " << name << "_ {\n  IsaPtr isa_;\n";
  // Flatten ancestor instance fields then own.
  {
    std::vector<std::string> chain;
    std::string p = c.parent;
    while (!p.empty()) {
      chain.push_back(p);
      auto it = ctx.classes.find(p);
      if (it == ctx.classes.end()) {
        break;
      }
      p = it->second.decl->parent;
    }
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
      for (const Field& f : ctx.classes[*it].decl->fields) {
        if (!f.is_static) {
          ctx.out << "  id " << f.name << ";\n";
        }
      }
    }
  }
  for (const Field& f : c.fields) {
    if (!f.is_static) {
      ctx.out << "  id " << f.name << ";\n";
    }
  }
  ctx.out << "};\n\n";
  ctx.out << "static VTable* " << name << "_vtable = nullptr;\n\n";

  if (info->has_static) {
    ctx.out << "struct " << name << "Class_ {\n  IsaPtr isa_;\n";
    for (const Field& f : c.fields) {
      if (f.is_static) {
        ctx.out << "  id " << f.name << ";\n";
      }
    }
    ctx.out << "};\n";
    ctx.out << "static " << name << "Class_ g_" << name << "Class;\n";
    ctx.out << "static VTable* " << name << "Class_vtable = nullptr;\n";
    ctx.out << "static bool g_" << name << "Class_inited = false;\n\n";
  }

  bool has_ctor = false;
  for (const Method& m : c.methods) {
    if (m.name.empty()) {
      has_ctor = true;
      ctx.out << "static id " << name << "__init(id self, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id p" << i;
      }
      ctx.out << ");\n";
      ctx.out << "static id " << name << "__new(id, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id p" << i;
      }
      ctx.out << ");\n";
    } else {
      const std::string mangled = mangle_method(m.name, m.params.size());
      ctx.out << "static id " << name << "__" << mangled << "(id self, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id p" << i;
      }
      ctx.out << ");\n";
    }
  }
  if (!has_ctor) {
    // synthesize zero-arg ctor for field defaults
    ctx.out << "static id " << name << "__init(id self, int selector);\n";
    ctx.out << "static id " << name << "__new(id, int selector);\n";
  }
  for (const Field& f : c.fields) {
    if (f.is_static) {
      continue;
    }
    if (f.readable || f.accessible) {
      ctx.out << "static id " << name << "__" << mangle_getter(f.name)
              << "(id self, int selector);\n";
    }
    if (f.accessible) {
      ctx.out << "static id " << name << "__" << mangle_setter(f.name)
              << "(id self, int selector, id v);\n";
    }
  }
  ctx.out << "\n";

  emit_accessor_methods(ctx, c);

  ctx.current_class = info;

  auto emit_ctor = [&](const Method* m) {
    // __init: run on an already-allocated `self` (used by `super(...)`).
    ctx.out << "static id\n" << name << "__init(id self, int selector";
    if (m) {
      for (size_t i = 0; i < m->params.size(); ++i) {
        ctx.out << ", id " << m->params[i];
      }
    }
    ctx.out << ")\n{\n  (void)selector;\n";
    // instance field defaults (own fields only)
    for (const Field& f : c.fields) {
      if (f.is_static || !f.init) {
        continue;
      }
      const std::string t = ctx.fresh("t");
      ctx.out << "  id " << t << ";\n";
      emit_expr_top(ctx, *f.init, t);
      ctx.out << "  body<" << name << "_>(self)->" << f.name << " = " << t << ";\n";
    }
    info->param_names.clear();
    if (m) {
      for (const std::string& p : m->params) {
        info->param_names.insert(p);
      }
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      emit_body(ctx, m->body, tmp, true);
    } else {
      ctx.out << "  return self;\n";
    }
    ctx.out << "}\n\n";

    // __new: allocate + set isa + __init
    ctx.out << "static id\n" << name << "__new(id, int selector";
    if (m) {
      for (size_t i = 0; i < m->params.size(); ++i) {
        ctx.out << ", id " << m->params[i];
      }
    }
    ctx.out << ")\n{\n  (void)selector;\n";
    ctx.out << "  " << name << "_* self_b = alloc<" << name << "_>();\n";
    ctx.out << "  id self = as_id(self_b);\n";
    ctx.out << "  zefc_set_isa(self_b, " << name << "_vtable);\n";
    ctx.out << "  return " << name << "__init(self, selector";
    if (m) {
      for (size_t i = 0; i < m->params.size(); ++i) {
        ctx.out << ", " << m->params[i];
      }
    }
    ctx.out << ");\n";
    ctx.out << "}\n\n";
  };

  bool emitted_ctor = false;
  for (const Method& m : c.methods) {
    info->param_names.clear();
    for (const std::string& p : m.params) {
      info->param_names.insert(p);
    }
    if (m.name.empty()) {
      emit_ctor(&m);
      emitted_ctor = true;
    } else {
      const std::string mangled = mangle_method(m.name, m.params.size());
      ctx.out << "static id\n" << name << "__" << mangled << "(id self, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id " << m.params[i];
      }
      ctx.out << ")\n{\n  (void)selector;\n";
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      emit_body(ctx, m.body, tmp, false);
      ctx.out << "}\n\n";
    }
  }
  if (!emitted_ctor) {
    emit_ctor(nullptr);
  }

  ctx.out << "static void\nensure_" << name << "()\n{\n";
  ctx.out << "  if (" << name << "_vtable) {\n    return;\n  }\n";
  if (!c.parent.empty()) {
    ctx.out << "  ensure_" << c.parent << "();\n";
  }
  ctx.out << "  " << name << "_vtable = vtable_create();\n";
  // Inherit parent method slots (override below).
  if (!c.parent.empty()) {
    auto pit = ctx.classes.find(c.parent);
    if (pit != ctx.classes.end()) {
      const ClassDecl& parent = *pit->second.decl;
      for (const Method& pm : parent.methods) {
        if (pm.name.empty() || pm.is_static) {
          continue;
        }
        const std::string mangled = mangle_method(pm.name, pm.params.size());
        ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\"" << mangled
                << "\"), " << c.parent << "__" << mangled << ");\n";
      }
      for (const Field& f : parent.fields) {
        if (f.is_static) {
          continue;
        }
        if (f.readable || f.accessible) {
          ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
                  << mangle_getter(f.name) << "\"), " << c.parent << "__"
                  << mangle_getter(f.name) << ");\n";
        }
        if (f.accessible) {
          ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
                  << mangle_setter(f.name) << "\"), " << c.parent << "__"
                  << mangle_setter(f.name) << ");\n";
        }
      }
    }
  }
  for (const Field& f : c.fields) {
    if (f.is_static) {
      continue;
    }
    if (f.readable || f.accessible) {
      ctx.out << "  field_register_get(" << name << "_vtable, selector_intern(\""
              << mangle_getter(f.name) << "\"), offsetof(" << name << "_, " << f.name
              << "));\n";
      ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
              << mangle_getter(f.name) << "\"), " << name << "__" << mangle_getter(f.name)
              << ");\n";
    }
    if (f.accessible) {
      ctx.out << "  field_register_set(" << name << "_vtable, selector_intern(\""
              << mangle_setter(f.name) << "\"), offsetof(" << name << "_, " << f.name
              << "));\n";
      ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
              << mangle_setter(f.name) << "\"), " << name << "__" << mangle_setter(f.name)
              << ");\n";
    }
  }
  for (const Method& m : c.methods) {
    if (m.name.empty()) {
      continue;
    }
    if (m.is_static) {
      continue;
    }
    const std::string mangled = mangle_method(m.name, m.params.size());
    ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\"" << mangled << "\"), "
            << name << "__" << mangled << ");\n";
  }
  if (info->has_static) {
    ctx.out << "  if (!g_" << name << "Class_inited) {\n";
    ctx.out << "    " << name << "Class_vtable = vtable_create();\n";
    ctx.out << "    zefc_set_isa(&g_" << name << "Class, " << name << "Class_vtable);\n";
    for (const Method& m : c.methods) {
      if (!m.is_static || m.name.empty()) {
        continue;
      }
      const std::string mangled = mangle_method(m.name, m.params.size());
      ctx.out << "    vtable_set(" << name << "Class_vtable, selector_intern(\"" << mangled
              << "\"), " << name << "__" << mangled << ");\n";
    }
    for (const Field& f : c.fields) {
      if (!f.is_static || !f.init) {
        continue;
      }
      const std::string t = ctx.fresh("t");
      ctx.out << "    id " << t << ";\n";
      emit_expr_top(ctx, *f.init, t);
      ctx.out << "    g_" << name << "Class." << f.name << " = " << t << ";\n";
    }
    ctx.out << "    g_" << name << "Class_inited = true;\n";
    ctx.out << "  }\n";
  }
  ctx.out << "}\n\n";

  ctx.current_class = nullptr;
}

} // namespace

std::string
codegen_cpp(const Program& program, const std::string& source_path)
{
  Ctx ctx;
  collect_classes(ctx, program);

  ctx.out << "// Generated by zefc from " << source_path << "\n";
  ctx.out << "// object_dispatch selected at C++ compile time (-DZEFC_OBJECT_DISPATCH).\n\n";
  ctx.out << "#include \"zefc/array_api.hpp\"\n";
  ctx.out << "#include \"zefc/dispatch.hpp\"\n";
  ctx.out << "#include \"zefc/field_ic.hpp\"\n";
  ctx.out << "#include \"zefc/int_api.hpp\"\n";
  ctx.out << "#include \"zefc/io.hpp\"\n";
  ctx.out << "#include \"zefc/known_selectors.hpp\"\n";
  ctx.out << "#include \"zefc/runtime.hpp\"\n";
  ctx.out << "#include \"zefc/runtime_bootstrap.hpp\"\n";
  ctx.out << "#include \"zefc/string_api.hpp\"\n\n";
  ctx.out << "#include <cstddef>\n\n";
  ctx.out << "using namespace zefc;\n\n";
  ctx.out << "namespace {\n\n";

  for (const Stmt& s : program.stmts) {
    if (s.kind == Stmt::Kind::VarDecl) {
      ctx.toplevel_vars.push_back(s.var_name);
      ctx.out << "static id " << s.var_name << ";\n";
    }
  }
  if (!ctx.toplevel_vars.empty()) {
    ctx.out << "\n";
  }

  for (const Stmt& s : program.stmts) {
    if (s.kind == Stmt::Kind::Class) {
      emit_class(ctx, s.class_decl);
    }
  }

  // Emit top-level functions into a buffer so closure prelude can be placed first.
  std::ostringstream funcs_out;
  {
    std::ostringstream saved;
    saved << ctx.out.str();
    ctx.out.str("");
    ctx.out.clear();
    for (const Stmt& s : program.stmts) {
      if (s.kind != Stmt::Kind::Func) {
        continue;
      }
      const FuncDecl& f = s.func_decl;
      // Duplicate param names: only the last binding is visible (Zef); rename earlier ones.
      std::vector<std::string> cpp_params;
      cpp_params.reserve(f.params.size());
      for (size_t i = 0; i < f.params.size(); ++i) {
        bool shadowed = false;
        for (size_t j = i + 1; j < f.params.size(); ++j) {
          if (f.params[j] == f.params[i]) {
            shadowed = true;
            break;
          }
        }
        if (shadowed) {
          cpp_params.push_back(f.params[i] + "__" + std::to_string(i));
        } else {
          cpp_params.push_back(f.params[i]);
        }
      }
      ctx.out << "static id\nfn_" << f.name << "(";
      for (size_t i = 0; i < cpp_params.size(); ++i) {
        if (i) {
          ctx.out << ", ";
        }
        ctx.out << "id " << cpp_params[i];
      }
      ctx.out << ")\n{\n";
      for (size_t i = 0; i < cpp_params.size(); ++i) {
        if (cpp_params[i] != f.params[i]) {
          ctx.out << "  (void)" << cpp_params[i] << ";\n";
        }
      }
      ctx.current_class = nullptr;
      ctx.env_params = ctx.toplevel_vars;
      for (const std::string& p : cpp_params) {
        // Only non-shadowed names (original names) are usable in the body.
        if (p.find("__") == std::string::npos) {
          ctx.env_params.push_back(p);
        }
      }
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      emit_body(ctx, f.body, tmp, false);
      ctx.env_params.clear();
      ctx.out << "}\n\n";
    }
    funcs_out << ctx.out.str();
    ctx.out.str("");
    ctx.out.clear();
    ctx.out << saved.str();
  }

  ctx.out << ctx.prelude.str();
  // Reset prelude; top-level `my` / expr may emit more closures into it.
  ctx.prelude.str("");
  ctx.prelude.clear();
  ctx.out << funcs_out.str();

  // Emit main body first so any top-level lambdas land in prelude.
  std::ostringstream main_body;
  {
    std::ostringstream saved;
    saved << ctx.out.str();
    ctx.out.str("");
    ctx.out.clear();
    ctx.out << "  runtime_package_init();\n";
    for (const auto& kv : ctx.classes) {
      ctx.out << "  ensure_" << kv.first << "();\n";
    }
    ctx.env_params = ctx.toplevel_vars;
    for (const Stmt& s : program.stmts) {
      if (s.kind == Stmt::Kind::VarDecl) {
        ctx.current_class = nullptr;
        if (s.expr) {
          emit_expr_top(ctx, *s.expr, s.var_name);
        } else {
          ctx.out << "  " << s.var_name << " = null_id();\n";
        }
        // Already in toplevel_vars / env_params.
      } else if (s.kind == Stmt::Kind::Expr) {
        ctx.current_class = nullptr;
        // Bare function name → call
        if (s.expr->kind == Expr::Kind::Ident && ctx.functions.count(s.expr->text)) {
          ctx.out << "  (void)fn_" << s.expr->text << "();\n";
          continue;
        }
        const std::string tmp = ctx.fresh("t");
        ctx.out << "  id " << tmp << ";\n";
        emit_expr_top(ctx, *s.expr, tmp);
      }
    }
    ctx.out << "  return 0;\n";
    main_body << ctx.out.str();
    ctx.out.str("");
    ctx.out.clear();
    ctx.out << saved.str();
  }

  ctx.out << ctx.prelude.str();
  ctx.out << "} // namespace\n\n";
  ctx.out << "int\nmain()\n{\n";
  ctx.out << main_body.str();
  ctx.out << "}\n";
  return ctx.out.str();
}

} // namespace compiler
} // namespace zefc
