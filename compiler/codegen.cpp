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
  return "ZEFC_SITE(\"" + mangled + "\")";
}

struct ClassInfo {
  const ClassDecl* decl = nullptr;
  std::unordered_set<std::string> field_names;
  std::unordered_set<std::string> param_names;
  bool has_static = false;
};

struct Ctx {
  std::ostringstream out;
  std::unordered_map<std::string, ClassInfo> classes;
  const ClassInfo* current_class = nullptr;
  int tmp = 0;

  std::string fresh(const char* prefix)
  {
    return std::string(prefix) + std::to_string(tmp++);
  }
};

void emit_expr(Ctx& ctx, const Expr& e, const std::string& dst);
void emit_expr_top(Ctx& ctx, const Expr& e, const std::string& dst);

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
emit_expr(Ctx& ctx, const Expr& e, const std::string& dst)
{
  switch (e.kind) {
  case Expr::Kind::Ident: {
    if (is_field(ctx, e.text)) {
      const std::string& cls = ctx.current_class->decl->name;
      ctx.out << "  " << dst << " = body<" << cls << "_>(self)->" << e.text << ";\n";
    } else {
      ctx.out << "  " << dst << " = " << e.text << ";\n";
    }
    return;
  }
  case Expr::Kind::String:
    ctx.out << "  " << dst << " = String__from_utf8(\"" << mangle_escape(e.text) << "\");\n";
    return;
  case Expr::Kind::Number:
    ctx.out << "  " << dst << " = Int__from_i64(" << e.text << ");\n";
    return;
  case Expr::Kind::Dot: {
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
    if (e.text != "+") {
      throw std::runtime_error("unsupported binary op: " + e.text);
    }
    const std::string a = ctx.fresh("t");
    const std::string b = ctx.fresh("t");
    ctx.out << "  id " << a << ";\n  id " << b << ";\n";
    emit_expr(ctx, *e.lhs, a);
    emit_expr(ctx, *e.rhs, b);
    ctx.out << "  " << dst << " = ZEFC_SEND1(" << a << ", ZEFC_SEL_add_o, " << b << ");\n";
    return;
  }
  case Expr::Kind::Assign: {
    const std::string rhs = ctx.fresh("t");
    ctx.out << "  id " << rhs << ";\n";
    emit_expr(ctx, *e.rhs, rhs);
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
    if (!e.lhs || e.lhs->kind != Expr::Kind::Ident) {
      throw std::runtime_error("assignment target must be an identifier or field");
    }
    const std::string& name = e.lhs->text;
    if (is_field(ctx, name)) {
      const std::string& cls = ctx.current_class->decl->name;
      ctx.out << "  body<" << cls << "_>(self)->" << name << " = " << rhs << ";\n";
      ctx.out << "  " << dst << " = " << rhs << ";\n";
    } else {
      ctx.out << "  " << name << " = " << rhs << ";\n";
      ctx.out << "  " << dst << " = " << rhs << ";\n";
    }
    return;
  }
  case Expr::Kind::Call: {
    if (!e.lhs || e.lhs->kind != Expr::Kind::Ident) {
      throw std::runtime_error("only simple calls supported in this milestone");
    }
    const std::string& callee = e.lhs->text;
    std::vector<std::string> arg_tmps;
    for (size_t i = 0; i < e.args.size(); ++i) {
      arg_tmps.push_back(ctx.fresh("t"));
      ctx.out << "  id " << arg_tmps.back() << ";\n";
      emit_expr(ctx, *e.args[i], arg_tmps.back());
    }
    if (callee == "println") {
      if (arg_tmps.size() != 1) {
        throw std::runtime_error("println expects 1 argument");
      }
      ctx.out << "  println(" << arg_tmps[0] << ");\n";
      ctx.out << "  " << dst << " = null_id();\n";
      return;
    }
    if (ctx.classes.count(callee)) {
      ctx.out << "  " << dst << " = " << callee << "__new(null_id(), 0";
      for (const std::string& a : arg_tmps) {
        ctx.out << ", " << a;
      }
      ctx.out << ");\n";
      return;
    }
    throw std::runtime_error("unknown call target: " + callee);
  }
  }
  throw std::runtime_error("unhandled expression kind");
}

void
emit_expr_top(Ctx& ctx, const Expr& e, const std::string& dst)
{
  if (e.kind == Expr::Kind::Call && e.lhs && e.lhs->kind == Expr::Kind::Dot) {
    const std::string recv = ctx.fresh("t");
    ctx.out << "  id " << recv << ";\n";
    emit_expr(ctx, *e.lhs->lhs, recv);
    std::vector<std::string> arg_tmps;
    for (size_t i = 0; i < e.args.size(); ++i) {
      arg_tmps.push_back(ctx.fresh("t"));
      ctx.out << "  id " << arg_tmps.back() << ";\n";
      emit_expr(ctx, *e.args[i], arg_tmps.back());
    }
    emit_send(ctx, recv, mangle_method(e.lhs->text, e.args.size()), arg_tmps, dst);
    return;
  }
  emit_expr(ctx, e, dst);
}

void
emit_body(Ctx& ctx, const std::vector<ExprPtr>& body, const std::string& result_dst,
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
    const std::string t = ctx.fresh("t");
    ctx.out << "  id " << t << ";\n";
    emit_expr_top(ctx, *body[i], t);
  }
  emit_expr_top(ctx, *body.back(), result_dst);
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
    ctx.classes[s.class_decl.name] = std::move(info);
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
    const size_t nparams = m ? m->params.size() : 0;
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
    // instance field defaults
    for (const Field& f : c.fields) {
      if (f.is_static || !f.init) {
        continue;
      }
      const std::string t = ctx.fresh("t");
      ctx.out << "  id " << t << ";\n";
      emit_expr_top(ctx, *f.init, t);
      ctx.out << "  self_b->" << f.name << " = " << t << ";\n";
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
    (void)nparams;
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
  ctx.out << "  " << name << "_vtable = vtable_create();\n";
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
    const std::string mangled = mangle_method(m.name, m.params.size());
    ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\"" << mangled << "\"), "
            << name << "__" << mangled << ");\n";
  }
  if (info->has_static) {
    ctx.out << "  if (!g_" << name << "Class_inited) {\n";
    ctx.out << "    " << name << "Class_vtable = vtable_create();\n";
    ctx.out << "    zefc_set_isa(&g_" << name << "Class, " << name << "Class_vtable);\n";
    for (const Field& f : c.fields) {
      if (!f.is_static || !f.init) {
        continue;
      }
      const std::string t = ctx.fresh("t");
      ctx.out << "    id " << t << ";\n";
      // init expr at ensure time (top-level constants)
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
    if (s.kind == Stmt::Kind::Class) {
      emit_class(ctx, s.class_decl);
    }
  }

  ctx.out << "} // namespace\n\n";
  ctx.out << "int\nmain()\n{\n";
  ctx.out << "  runtime_package_init();\n";
  for (const auto& kv : ctx.classes) {
    ctx.out << "  ensure_" << kv.first << "();\n";
  }
  for (const Stmt& s : program.stmts) {
    if (s.kind == Stmt::Kind::VarDecl) {
      ctx.out << "  id " << s.var_name << ";\n";
      ctx.current_class = nullptr;
      emit_expr_top(ctx, *s.expr, s.var_name);
    } else if (s.kind == Stmt::Kind::Expr) {
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      ctx.current_class = nullptr;
      emit_expr_top(ctx, *s.expr, tmp);
    }
  }
  ctx.out << "  return 0;\n}\n";
  return ctx.out.str();
}

} // namespace compiler
} // namespace zefc
