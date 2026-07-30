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
  // Orchard-style: name + '_' + arity encoding with 'o' per object arg.
  std::string m = name + "_";
  for (size_t i = 0; i < arity; ++i) {
    m.push_back('o');
  }
  if (arity == 0) {
    m.push_back('o'); // zero-arg instance method still ends with _o in ZefC smoke
  }
  return m;
}

std::string
mangle_getter(const std::string& field)
{
  return field + "_o";
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
  std::unordered_set<std::string> param_names; // filled per-method during emit
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
emit_expr(Ctx& ctx, const Expr& e, const std::string& dst)
{
  switch (e.kind) {
  case Expr::Kind::Ident: {
    if (is_field(ctx, e.text)) {
      const std::string& cls = ctx.current_class->decl->name;
      ctx.out << "  " << dst << " = ZEFC_IC_GET(self, " << sel_expr(mangle_getter(e.text))
              << ", " << cls << "_, " << e.text << ");\n";
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
    const std::string recv = ctx.fresh("t");
    ctx.out << "  id " << recv << ";\n";
    emit_expr(ctx, *e.lhs, recv);
    // member send: zero-arg method (toString) or field get on other object — treat as send
    const std::string mangled = mangle_method(e.text, 0);
    ctx.out << "  " << dst << " = ZEFC_SEND0(" << recv << ", " << sel_expr(mangled) << ");\n";
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
    if (!e.lhs || e.lhs->kind != Expr::Kind::Ident) {
      throw std::runtime_error("assignment target must be an identifier");
    }
    const std::string& name = e.lhs->text;
    const std::string rhs = ctx.fresh("t");
    ctx.out << "  id " << rhs << ";\n";
    emit_expr(ctx, *e.rhs, rhs);
    if (is_field(ctx, name)) {
      const std::string& cls = ctx.current_class->decl->name;
      // readable-only: still allow ctor assign via direct store + optional IC set
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
      // Class ctor
      if (arg_tmps.size() != 1) {
        // support N args later; hello uses 1
        throw std::runtime_error("ctor arity: only 1-arg ctors in this milestone");
      }
      ctx.out << "  " << dst << " = " << callee << "__new(null_id(), 0, " << arg_tmps[0]
              << ");\n";
      return;
    }
    // Method send on implicit? not supported — need recv.method(args) as Dot+Call
    throw std::runtime_error("unknown call target: " + callee);
  }
  }
  throw std::runtime_error("unhandled expression kind");
}

void
emit_expr_top(Ctx& ctx, const Expr& e, const std::string& dst)
{
  // Handle recv.name(args): parser builds Call with lhs=Dot
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
    const std::string mangled = mangle_method(e.lhs->text, e.args.size());
    if (e.args.empty()) {
      ctx.out << "  " << dst << " = ZEFC_SEND0(" << recv << ", " << sel_expr(mangled) << ");\n";
    } else if (e.args.size() == 1) {
      ctx.out << "  " << dst << " = ZEFC_SEND1(" << recv << ", " << sel_expr(mangled) << ", "
              << arg_tmps[0] << ");\n";
    } else {
      throw std::runtime_error("send arity >1 not in this milestone");
    }
    return;
  }
  emit_expr(ctx, e, dst);
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
      info.field_names.insert(f.name);
    }
    ctx.classes[s.class_decl.name] = std::move(info);
  }
}

void
emit_class(Ctx& ctx, const ClassDecl& c)
{
  const std::string& name = c.name;
  ctx.out << "struct " << name << "_ {\n  IsaPtr isa_;\n";
  for (const Field& f : c.fields) {
    ctx.out << "  id " << f.name << ";\n";
  }
  ctx.out << "};\n\n";
  ctx.out << "static VTable* " << name << "_vtable = nullptr;\n\n";

  // Forward decls
  for (const Method& m : c.methods) {
    if (m.name.empty()) {
      ctx.out << "static id " << name << "__new(id, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id " << m.params[i];
      }
      ctx.out << ");\n";
    } else {
      const std::string mangled = mangle_method(m.name, m.params.size());
      ctx.out << "static id " << name << "__" << mangled << "(id self, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id a" << i;
      }
      ctx.out << ");\n";
    }
  }
  ctx.out << "\n";

  ClassInfo* info = &ctx.classes[name];
  ctx.current_class = info;

  for (const Method& m : c.methods) {
    info->param_names.clear();
    for (const std::string& p : m.params) {
      info->param_names.insert(p);
    }

    if (m.name.empty()) {
      // constructor
      ctx.out << "static id\n" << name << "__new(id, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id " << m.params[i];
      }
      ctx.out << ")\n{\n  (void)selector;\n";
      ctx.out << "  " << name << "_* self_b = alloc<" << name << "_>();\n";
      ctx.out << "  id self = as_id(self_b);\n";
      ctx.out << "  zefc_set_isa(self_b, " << name << "_vtable);\n";
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      emit_expr_top(ctx, *m.body, tmp);
      ctx.out << "  return self;\n}\n\n";
    } else {
      const std::string mangled = mangle_method(m.name, m.params.size());
      ctx.out << "static id\n" << name << "__" << mangled << "(id self, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id " << m.params[i];
      }
      ctx.out << ")\n{\n  (void)selector;\n";
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      emit_expr_top(ctx, *m.body, tmp);
      ctx.out << "  return " << tmp << ";\n}\n\n";
    }
  }

  // ensure / vtable init
  ctx.out << "static void\nensure_" << name << "()\n{\n";
  ctx.out << "  if (" << name << "_vtable) {\n    return;\n  }\n";
  ctx.out << "  " << name << "_vtable = vtable_create();\n";
  for (const Field& f : c.fields) {
    if (f.readable || f.accessible) {
      ctx.out << "  field_register_get(" << name << "_vtable, selector_intern(\""
              << mangle_getter(f.name) << "\"), offsetof(" << name << "_, " << f.name
              << "));\n";
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
    if (s.kind == Stmt::Kind::Expr) {
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      ctx.current_class = nullptr;
      emit_expr_top(ctx, *s.expr, tmp);
      (void)tmp;
    }
  }
  ctx.out << "  return 0;\n}\n";
  return ctx.out.str();
}

} // namespace compiler
} // namespace zefc
