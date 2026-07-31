#include "codegen.hpp"
#include "version.hpp"

#include <functional>
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

// Struct fields named `id` (etc.) must not use bare `id` as the type — it shadows
// `zefc::id` for later members. Always qualify the type in layouts.
const char* kIdType = "::zefc::id";

// Locals/params named `id` would shadow the type for the whole function body.
std::string
local_sym(const std::string& name)
{
  if (name == "id") {
    return "z_id";
  }
  return name;
}

std::string
truthy_expr(const std::string& v)
{
  // Null falsy; int 0 falsy (despite odd-pointer tagging); objects/doubles truthy if non-null.
  return "((" + v + ") && !(id_is_int32(" + v + ") && Int__to_i64(" + v + ") == 0))";
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

// Static methods live as Class__sel so they can share names with instance methods.
std::string
method_sym(const std::string& cls, bool is_static, const std::string& mangled)
{
  return is_static ? (cls + "Class__" + mangled) : (cls + "__" + mangled);
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
  std::string cpp_name; // C++ symbol prefix (may differ for type-nested classes)
  std::unordered_set<std::string> field_names;
  std::unordered_set<std::string> private_fields; // private readable/accessible / method names
  std::unordered_set<std::string> param_names;
  std::unordered_set<std::string> method_names; // zero-arg instance methods (bare Ident call)
  std::unordered_map<std::string, size_t> methods; // instance method name → arity
  std::unordered_set<std::string> static_methods0; // zero-arg static methods
  // Type-nested: source name → nested class cpp_name
  std::unordered_map<std::string, std::string> nested_static;
  std::unordered_map<std::string, std::string> nested_instance;
  bool has_static = false;
  bool has_static_call0 = false; // static fn call with 0 params
  bool synth_nested_call = false; // Foo() forwards to static nested class `call`
  bool has_instance_ctor = false; // explicit `fn (...)` / `fn { }` constructor
  bool private_ctor = false;      // constructor is `private fn ...`
  // Enclosing type-nested class (Foo_Bar → Foo). Empty if top-level / fn-local.
  std::string enclosing_cpp;
  bool nest_is_static = false;
  bool needs_outer = false;      // instance-nested: live outer instance pointer
  bool has_outer_field = false;  // emit zefc_outer on this layout (not inherited)
  bool is_type_nested = false;   // needs Class object + Class__call_o
  bool dynamic_parent = false;   // parent chosen once via parent_expr at ensure_
};

struct FuncInfo {
  const FuncDecl* decl = nullptr;
  size_t arity = 0;
};

struct PackageInfo {
  std::string path;     // foo, foo_bar
  std::string cpp_name; // Pkg_foo
  std::unordered_map<std::string, std::string> classes;  // Bar -> foo_Bar
  std::unordered_map<std::string, const FuncDecl*> funcs;
  std::unordered_map<std::string, std::string> packages; // bar -> Pkg_foo_bar
  std::vector<const Field*> fields;
  const std::vector<BlockItem>* ctor_body = nullptr;
  bool has_ctor = false;
};

enum class ImportKind { Class, Func, Package };

struct ImportBinding {
  ImportKind kind = ImportKind::Class;
  std::string cpp;    // class cpp, or package cpp
  std::string member; // func name when Kind::Func
  size_t arity = 0;
};

struct Ctx {
  std::ostringstream out;
  std::ostringstream prelude; // closure / nested-class types at namespace scope
  // Top-level class method bodies (emitted after prelude so lambdas resolve).
  std::ostringstream class_methods;
  std::unordered_map<std::string, ClassInfo> classes;
  std::unordered_map<std::string, FuncInfo> functions;
  std::unordered_map<std::string, PackageInfo> packages; // keyed by cpp_name
  std::unordered_map<std::string, std::string> package_cpp; // source name -> cpp (top-level)
  std::unordered_map<std::string, ImportBinding> imports;
  // While emitting package field inits / methods: short class names in this package.
  std::unordered_map<std::string, std::string> package_locals;
  std::unordered_map<std::string, size_t> package_methods; // name -> arity
  // Active package (cpp name) whose fields are visible as bare idents.
  std::string package_scope_cpp;
  std::unordered_set<std::string> package_scope_fields;
  std::vector<std::string> toplevel_vars;
  const ClassInfo* current_class = nullptr;
  bool in_static_method = false;
  std::vector<std::string> env_params;
  // `my` locals (and params) that shadow instance fields for the current method.
  std::unordered_set<std::string> local_vars;
  // Locals bound to zero-arg lambdas (`fn name { ... }`) — bare Ident calls them.
  std::unordered_set<std::string> local_fn0;
  // Nested class names in scope → capture field names on the class object.
  std::unordered_map<std::string, std::vector<std::string>> local_classes;
  std::unordered_map<std::string, const ClassDecl*> local_class_decls;
  std::unordered_set<std::string> local_class_static_init;
  std::unordered_set<std::string> nested_emitted;
  // Real class whose method contains this lambda (for bare private/static methods).
  const ClassInfo* lambda_host = nullptr;
  // While emitting a nested-class constructor (class.call):
  std::string nest_capture_type;   // e.g. "WTFClass_"
  std::string nest_capture_self;   // C++ expr for class object
  std::unordered_set<std::string> nest_captures;
  std::string source_path;
  int last_line = -1;
  int tmp = 0;
  int closure_id = 0;
  int loop_depth = 0;
  bool allow_return = false;
  bool saw_load = false;
  // True inside functions/lambdas/methods, or brace blocks that declare `my`.
  // When true, import(foo) overwrites local bindings; when false, existing names win.
  bool import_binds = false;
  // Soft-fail unknown names (dynamic-parent else branches not taken at runtime).
  bool allow_unresolved = false;
  // package name → members exported by a loaded module (e.g. foo → f,x).
  std::unordered_map<std::string, std::vector<std::string>> loaded_package_members;

  std::string fresh(const char* prefix)
  {
    return std::string(prefix) + std::to_string(tmp++);
  }
};

void
emit_line(Ctx& ctx, int line)
{
  if (line <= 0 || line == ctx.last_line) {
    return;
  }
  ctx.last_line = line;
  ctx.out << "#line " << line << " \"" << mangle_escape(ctx.source_path) << "\"\n";
}

void emit_expr(Ctx& ctx, const Expr& e, const std::string& dst);
void emit_expr_top(Ctx& ctx, const Expr& e, const std::string& dst);
void emit_body(Ctx& ctx, const std::vector<BlockItem>& body, const std::string& result_dst,
               bool ctor_return_self);
void emit_block_item(Ctx& ctx, const BlockItem& item, const std::string* result_dst);
void emit_nested_class(Ctx& ctx, const ClassDecl& c, const std::vector<std::string>& captures);
void emit_class(Ctx& ctx, const ClassDecl& c, const std::string& cpp_name);
void collect_class_decl(Ctx& ctx, const ClassDecl& c, const std::string& cpp_name, bool top_level);
void resolve_nested_parents(ClassDecl& c, const std::string& cpp_name);
void collect_package_decl(Ctx& ctx, const PackageDecl& p, const std::string& path);
void emit_package(Ctx& ctx, const PackageInfo& pkg);
void emit_bound_nested_class(Ctx& ctx, const std::string& ncpp, const std::string& outer_self,
                             const std::string& dst);
bool try_resolve_enclosing(Ctx& ctx, const std::string& name, const std::string& dst);
void emit_send(Ctx& ctx, const std::string& recv, const std::string& mangled,
               const std::vector<std::string>& args, const std::string& dst);
void collect_free_class(const ClassDecl& c, const std::vector<std::string>& env,
                        std::vector<std::string>& captures);

std::string
class_cpp(const ClassInfo& info)
{
  return info.cpp_name.empty() ? info.decl->name : info.cpp_name;
}

bool
is_field(const Ctx& ctx, const std::string& name)
{
  if (!ctx.current_class) {
    return false;
  }
  // Params and `my` locals shadow instance fields (e.g. traverse's `my left`).
  if (ctx.current_class->param_names.count(name) || ctx.local_vars.count(name)) {
    return false;
  }
  return ctx.current_class->field_names.count(name);
}

void
emit_bound_nested_class(Ctx& ctx, const std::string& ncpp, const std::string& outer_self,
                        const std::string& dst)
{
  ctx.out << "  {\n";
  ctx.out << "    ensure_" << ncpp << "();\n";
  ctx.out << "    " << ncpp << "Class_* _cls = alloc<" << ncpp << "Class_>();\n";
  ctx.out << "    zefc_set_isa(_cls, " << ncpp << "Class_vtable);\n";
  ctx.out << "    _cls->zefc_outer = " << outer_self << ";\n";
  ctx.out << "    " << dst << " = as_id(_cls);\n";
  ctx.out << "  }\n";
}

bool
try_resolve_enclosing(Ctx& ctx, const std::string& name, const std::string& dst)
{
  if (!ctx.current_class || ctx.current_class->enclosing_cpp.empty()) {
    return false;
  }
  const ClassInfo& outer = ctx.classes.at(ctx.current_class->enclosing_cpp);
  const std::string& ocpp = class_cpp(outer);
  const std::string& ncpp = class_cpp(*ctx.current_class);
  if (ctx.current_class->nest_is_static) {
    for (const Field& f : outer.decl->fields) {
      if (f.is_static && f.name == name) {
        ctx.out << "  " << dst << " = g_" << ocpp << "Class." << name << ";\n";
        return true;
      }
    }
    if (outer.static_methods0.count(name)) {
      emit_send(ctx, "as_id(&g_" + ocpp + "Class)", mangle_method(name, 0), {}, dst);
      return true;
    }
    // Instance members of the outer are not in scope; Zef reports this when the
    // method runs (static nested class bodies are still compilable).
    if (outer.field_names.count(name) || outer.method_names.count(name)) {
      ctx.out << "  zefc_error(\"cannot resolve get (call with no arguments) named " << name
              << "\");\n";
      ctx.out << "  " << dst << " = null_id();\n";
      return true;
    }
    return false;
  }
  // Instance-nested: live outer fields / zero-arg methods.
  if (outer.field_names.count(name) && !outer.nested_instance.count(name)) {
    ctx.out << "  " << dst << " = body<" << ocpp << "_>(body<" << ncpp
            << "_>(self)->zefc_outer)->" << name << ";\n";
    return true;
  }
  if (outer.method_names.count(name)) {
    emit_send(ctx, "body<" + ncpp + "_>(self)->zefc_outer", mangle_method(name, 0), {}, dst);
    return true;
  }
  return false;
}

// Subclass instances track whether `super`/`super(...)` ran (Zef parent Storage*).
void
emit_check_super_inited(Ctx& ctx, const std::string& cls)
{
  ctx.out << "  if (!body<" << cls << "_>(self)->zefc_super_inited) {\n";
  ctx.out << "    zefc_error(\"attempt to use unconstructed class\");\n";
  ctx.out << "  }\n";
}

void
emit_mark_super_inited(Ctx& ctx)
{
  if (!ctx.current_class) {
    return;
  }
  const ClassDecl* d = ctx.current_class->decl;
  if (!d || (d->parent.empty() && !d->parent_expr && !ctx.current_class->dynamic_parent)) {
    return;
  }
  ctx.out << "  body<" << class_cpp(*ctx.current_class) << "_>(self)->zefc_super_inited = true;\n";
}

// True when immediate parent has no parent of its own (Foo in `class Bar : Foo`).
bool
parent_is_root(const ClassDecl& c,
               const std::function<const ClassDecl*(const std::string&)>& lookup)
{
  if (c.parent.empty()) {
    return true;
  }
  const ClassDecl* p = lookup(c.parent);
  return !p || p->parent.empty();
}

// Collect mangled selectors this class declares (so ancestors do not re-emit them).
void
collect_own_instance_selectors(const ClassDecl& c, std::unordered_set<std::string>& seen)
{
  for (const Method& m : c.methods) {
    if (!m.name.empty() && !m.is_static) {
      seen.insert(mangle_method(m.name, m.params.size()));
    }
  }
  for (const Field& f : c.fields) {
    if (f.is_static) {
      continue;
    }
    if (f.readable || f.accessible) {
      seen.insert(mangle_getter(f.name));
    }
    if (f.accessible) {
      seen.insert(mangle_setter(f.name));
    }
  }
  for (const ClassDecl::Nested& n : c.nested) {
    if (!n.is_static && !n.is_private) {
      seen.insert(mangle_getter(n.decl->name));
    }
  }
}

// Emit Sub__sel wrappers that forward to ImmediateParent__sel for every ancestor
// instance method/accessor not overridden (full chain, not only the direct parent).
void
emit_inherited_instance_forwards(Ctx& ctx, const ClassDecl& c, const std::string& name,
                                 const std::string& parent_cpp,
                                 const std::function<const ClassDecl*(const std::string&)>& lookup)
{
  std::unordered_set<std::string> seen;
  collect_own_instance_selectors(c, seen);
  // Zef checkConstructed runs on the defining class. Inherited forwards either
  // rely on Parent__method's check, or (when parent is a root class with no flag)
  // check that this leaf called super.
  const bool check_leaf = parent_is_root(c, lookup);
  std::string p = c.parent;
  while (!p.empty()) {
    const ClassDecl* anc = lookup(p);
    if (!anc) {
      break;
    }
    for (const Method& pm : anc->methods) {
      if (pm.name.empty() || pm.is_static) {
        continue;
      }
      const std::string mangled = mangle_method(pm.name, pm.params.size());
      if (!seen.insert(mangled).second) {
        continue;
      }
      ctx.out << "static id\n" << name << "__" << mangled << "(id self, int selector";
      for (size_t i = 0; i < pm.params.size(); ++i) {
        ctx.out << ", id a" << i;
      }
      ctx.out << ")\n{\n";
      if (check_leaf) {
        emit_check_super_inited(ctx, name);
      }
      ctx.out << "  return " << parent_cpp << "__" << mangled << "(self, selector";
      for (size_t i = 0; i < pm.params.size(); ++i) {
        ctx.out << ", a" << i;
      }
      ctx.out << ");\n}\n\n";
    }
    for (const Field& f : anc->fields) {
      if (f.is_static) {
        continue;
      }
      if (f.readable || f.accessible) {
        const std::string g = mangle_getter(f.name);
        if (seen.insert(g).second) {
          ctx.out << "static id\n" << name << "__" << g << "(id self, int selector)\n{\n";
          if (check_leaf) {
            emit_check_super_inited(ctx, name);
          }
          ctx.out << "  return " << parent_cpp << "__" << g << "(self, selector);\n}\n\n";
        }
      }
      if (f.accessible) {
        const std::string s = mangle_setter(f.name);
        if (seen.insert(s).second) {
          ctx.out << "static id\n" << name << "__" << s
                  << "(id self, int selector, id v)\n{\n";
          if (check_leaf) {
            emit_check_super_inited(ctx, name);
          }
          ctx.out << "  return " << parent_cpp << "__" << s << "(self, selector, v);\n}\n\n";
        }
      }
    }
    for (const ClassDecl::Nested& n : anc->nested) {
      if (n.is_static || n.is_private) {
        continue;
      }
      const std::string g = mangle_getter(n.decl->name);
      if (!seen.insert(g).second) {
        continue;
      }
      ctx.out << "static id\n" << name << "__" << g << "(id self, int selector)\n{\n";
      if (check_leaf) {
        emit_check_super_inited(ctx, name);
      }
      ctx.out << "  return " << parent_cpp << "__" << g << "(self, selector);\n}\n\n";
    }
    p = anc->parent;
  }
}

void
emit_inherited_vtable_sets(Ctx& ctx, const ClassDecl& c, const std::string& name,
                           const std::function<const ClassDecl*(const std::string&)>& lookup)
{
  std::unordered_set<std::string> seen;
  collect_own_instance_selectors(c, seen);
  std::string p = c.parent;
  while (!p.empty()) {
    const ClassDecl* anc = lookup(p);
    if (!anc) {
      break;
    }
    for (const Method& pm : anc->methods) {
      if (pm.name.empty() || pm.is_static) {
        continue;
      }
      const std::string mangled = mangle_method(pm.name, pm.params.size());
      if (!seen.insert(mangled).second) {
        continue;
      }
      ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\"" << mangled
              << "\"), " << name << "__" << mangled << ");\n";
    }
    for (const Field& f : anc->fields) {
      if (f.is_static) {
        continue;
      }
      if (f.readable || f.accessible) {
        const std::string g = mangle_getter(f.name);
        if (seen.insert(g).second) {
          ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\"" << g
                  << "\"), " << name << "__" << g << ");\n";
        }
      }
      if (f.accessible) {
        const std::string s = mangle_setter(f.name);
        if (seen.insert(s).second) {
          ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\"" << s
                  << "\"), " << name << "__" << s << ");\n";
        }
      }
    }
    for (const ClassDecl::Nested& n : anc->nested) {
      if (n.is_static || n.is_private) {
        continue;
      }
      const std::string g = mangle_getter(n.decl->name);
      if (!seen.insert(g).second) {
        continue;
      }
      ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\"" << g << "\"), "
              << name << "__" << g << ");\n";
    }
    p = anc->parent;
  }
}

void
note_load_exports(Ctx& ctx, const std::string& path)
{
  // Hand-registered smoke modules (see loadable_modules.cpp).
  if (path == "stuff/package.zef") {
    ctx.loaded_package_members["foo"] = {"f", "x"};
  }
}

void
emit_store_name(Ctx& ctx, const std::string& name, const std::string& val)
{
  if (ctx.nest_captures.count(name)) {
    ctx.out << "  body<" << ctx.nest_capture_type << ">(" << ctx.nest_capture_self << ")->"
            << name << " = " << val << ";\n";
  } else if (is_field(ctx, name)) {
    ctx.out << "  body<" << class_cpp(*ctx.current_class) << "_>(self)->" << name << " = "
            << val << ";\n";
  } else {
    ctx.out << "  " << local_sym(name) << " = " << val << ";\n";
  }
}

bool
name_is_bound(const Ctx& ctx, const std::string& name)
{
  if (ctx.local_vars.count(name) || ctx.nest_captures.count(name)) {
    return true;
  }
  if (is_field(ctx, name)) {
    return true;
  }
  for (const std::string& v : ctx.toplevel_vars) {
    if (v == name) {
      return true;
    }
  }
  for (const std::string& v : ctx.env_params) {
    if (v == name) {
      return true;
    }
  }
  return false;
}

void
emit_import_bind(Ctx& ctx, const std::vector<std::string>& path)
{
  if (path.empty()) {
    throw std::runtime_error("empty import");
  }
  if (path.size() != 1) {
    throw std::runtime_error("runtime import of nested path not supported yet");
  }
  const std::string& pkg = path[0];
  std::vector<std::string> members;
  if (ctx.loaded_package_members.count(pkg)) {
    members = ctx.loaded_package_members[pkg];
  } else if (ctx.package_cpp.count(pkg)) {
    const PackageInfo& pi = ctx.packages.at(ctx.package_cpp.at(pkg));
    for (const auto& kv : pi.funcs) {
      if (kv.second->params.empty()) {
        members.push_back(kv.first);
      }
    }
    for (const Field* f : pi.fields) {
      if (f->readable || f->accessible) {
        members.push_back(f->name);
      }
    }
  } else {
    throw std::runtime_error("import of unknown package " + pkg);
  }
  for (const std::string& m : members) {
    if (!ctx.import_binds && name_is_bound(ctx, m)) {
      continue; // shared scope: existing bindings win
    }
    const std::string tmp = ctx.fresh("t");
    ctx.out << "  id " << tmp << " = package_slot_get(\"" << pkg << "\", \"" << m << "\");\n";
    if (!name_is_bound(ctx, m) && !is_field(ctx, m) && !ctx.nest_captures.count(m)) {
      ctx.out << "  id " << local_sym(m) << " = " << tmp << ";\n";
      ctx.local_vars.insert(m);
      ctx.env_params.push_back(m);
    } else {
      emit_store_name(ctx, m, tmp);
    }
  }
}

void
check_private_member_access(const Ctx& ctx, const std::string& name)
{
  bool private_elsewhere = false;
  bool allowed_here = false;
  for (const auto& kv : ctx.classes) {
    if (!kv.second.private_fields.count(name)) {
      continue;
    }
    if (ctx.current_class && class_cpp(*ctx.current_class) == class_cpp(kv.second)) {
      allowed_here = true;
    } else {
      private_elsewhere = true;
    }
  }
  if (private_elsewhere && !allowed_here) {
    throw std::runtime_error("no such method: " + name);
  }
}

void
check_private_class_member(const Ctx& ctx, const ClassInfo& ci, const std::string& name)
{
  if (!ci.private_fields.count(name)) {
    return;
  }
  if (ctx.current_class && class_cpp(*ctx.current_class) == class_cpp(ci)) {
    return;
  }
  throw std::runtime_error("no such method: " + name);
}

void
check_ctor_visible(const Ctx& ctx, const ClassInfo& ci)
{
  if (!ci.private_ctor) {
    return;
  }
  if (ctx.current_class && class_cpp(*ctx.current_class) == class_cpp(ci)) {
    return;
  }
  throw std::runtime_error(
      "cannot instantiate class, private constructor not visible at callsite");
}

bool
is_builtin_name(const std::string& name)
{
  return name == "println" || name == "print" || name == "load" || name == "error" ||
         name == "String" || name == "Array" || name == "int" || name == "double";
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
    // High-arity: direct table lookup (method is variadic).
    ctx.out << "  " << dst << " = zefc_method_at(" << recv << ", " << sel << ")(" << recv
            << ", " << sel;
    for (const std::string& a : args) {
      ctx.out << ", " << a;
    }
    ctx.out << ");\n";
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
    if (e.text != "null" && e.text != "true" && e.text != "false" && e.text != "this" &&
        e.text != "super") {
      add_cap(e.text);
    }
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
  case Expr::Kind::Block:
    if (e.lhs) {
      collect_free(*e.lhs, locals, env, captures);
    }
    {
      std::unordered_set<std::string> block_locals = locals;
      for (const BlockItem& item : e.body) {
        if (item.kind == BlockItem::Kind::VarDecl) {
          if (item.expr) {
            collect_free(*item.expr, block_locals, env, captures);
          }
          block_locals.insert(item.var_name);
        } else if (item.kind == BlockItem::Kind::Import) {
          // import binds package members; no free refs
        } else if (item.expr) {
          collect_free(*item.expr, block_locals, env, captures);
        }
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
      } else if (item.kind == BlockItem::Kind::Class && item.nested_class) {
        collect_free_class(*item.nested_class, nested_env, nested_caps);
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

// If an expression mentions a function-local class, the class object's capture
// fields must also be in scope (e.g. `fn test Bar(1,2)` needs Bar's captures).
void
collect_local_class_caps(const Expr& e, const std::unordered_set<std::string>& locals,
                         const std::vector<std::string>& env,
                         const std::unordered_map<std::string, std::vector<std::string>>& local_classes,
                         std::vector<std::string>& captures)
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
  auto add_class = [&](const std::string& name) {
    auto it = local_classes.find(name);
    if (it == local_classes.end()) {
      return;
    }
    for (const std::string& cap : it->second) {
      add_cap(cap);
    }
  };

  switch (e.kind) {
  case Expr::Kind::Ident:
    add_class(e.text);
    break;
  case Expr::Kind::Dot:
  case Expr::Kind::Unary:
  case Expr::Kind::Return:
    if (e.lhs) {
      collect_local_class_caps(*e.lhs, locals, env, local_classes, captures);
    }
    if (e.rhs) {
      collect_local_class_caps(*e.rhs, locals, env, local_classes, captures);
    }
    break;
  case Expr::Kind::Index:
  case Expr::Kind::Binary:
  case Expr::Kind::Assign:
    if (e.lhs) {
      collect_local_class_caps(*e.lhs, locals, env, local_classes, captures);
    }
    if (e.rhs) {
      collect_local_class_caps(*e.rhs, locals, env, local_classes, captures);
    }
    break;
  case Expr::Kind::Call:
  case Expr::Kind::ArrayLit:
    if (e.lhs) {
      collect_local_class_caps(*e.lhs, locals, env, local_classes, captures);
    }
    for (const auto& a : e.args) {
      if (a) {
        collect_local_class_caps(*a, locals, env, local_classes, captures);
      }
    }
    break;
  case Expr::Kind::While:
  case Expr::Kind::If:
  case Expr::Kind::Block:
    if (e.lhs) {
      collect_local_class_caps(*e.lhs, locals, env, local_classes, captures);
    }
    for (const BlockItem& item : e.body) {
      if (item.expr) {
        collect_local_class_caps(*item.expr, locals, env, local_classes, captures);
      }
    }
    for (const BlockItem& item : e.else_body) {
      if (item.expr) {
        collect_local_class_caps(*item.expr, locals, env, local_classes, captures);
      }
    }
    break;
  case Expr::Kind::Lambda: {
    std::unordered_set<std::string> nested_locals = locals;
    for (const std::string& p : e.params) {
      nested_locals.insert(p);
    }
    for (const BlockItem& item : e.body) {
      if (item.expr) {
        collect_local_class_caps(*item.expr, nested_locals, env, local_classes, captures);
      }
      if (item.kind == BlockItem::Kind::VarDecl) {
        nested_locals.insert(item.var_name);
      }
    }
    break;
  }
  default:
    break;
  }
}

void
collect_free_class(const ClassDecl& c, const std::vector<std::string>& env,
                   std::vector<std::string>& captures)
{
  std::unordered_set<std::string> class_locals;
  for (const Field& f : c.fields) {
    class_locals.insert(f.name);
  }
  for (const Method& m : c.methods) {
    if (!m.name.empty()) {
      class_locals.insert(m.name);
    }
  }
  for (const Method& m : c.methods) {
    std::unordered_set<std::string> scan_locals = class_locals;
    for (const std::string& p : m.params) {
      scan_locals.insert(p);
    }
    for (const BlockItem& bi : m.body) {
      if (bi.kind == BlockItem::Kind::VarDecl) {
        if (bi.expr) {
          collect_free(*bi.expr, scan_locals, env, captures);
        }
        scan_locals.insert(bi.var_name);
      } else if (bi.kind == BlockItem::Kind::Class && bi.nested_class) {
        collect_free_class(*bi.nested_class, env, captures);
      } else if (bi.expr) {
        collect_free(*bi.expr, scan_locals, env, captures);
      }
    }
  }
  for (const Field& f : c.fields) {
    if (f.init) {
      collect_free(*f.init, class_locals, env, captures);
    }
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
          collect_local_class_caps(*item.expr, scan_locals, ctx.env_params, ctx.local_classes,
                                   captures);
        }
        scan_locals.insert(item.var_name);
      } else if (item.kind == BlockItem::Kind::Class && item.nested_class) {
        collect_free_class(*item.nested_class, ctx.env_params, captures);
      } else if (item.expr) {
        collect_free(*item.expr, scan_locals, ctx.env_params, captures);
        collect_local_class_caps(*item.expr, scan_locals, ctx.env_params, ctx.local_classes,
                                 captures);
      }
    }
  }

  const ClassInfo* host = ctx.lambda_host;
  if (!host && ctx.current_class && ctx.current_class->decl &&
      ctx.current_class->decl->name.compare(0, 7, "Closure") != 0) {
    host = ctx.current_class;
  }
  if (host) {
    // Method names are invoked via captured host self, not by-value captures.
    std::vector<std::string> kept;
    for (const std::string& n : captures) {
      if (!host->method_names.count(n) && !host->static_methods0.count(n)) {
        kept.push_back(n);
      }
    }
    captures.swap(kept);
  }

  const int id = ctx.closure_id++;
  const std::string cname = "Closure" + std::to_string(id);

  ctx.prelude << "struct " << cname << "_ {\n  IsaPtr isa_;\n";
  if (host) {
    ctx.prelude << "  " << kIdType << " zefc_host;\n";
  }
  for (const std::string& cap : captures) {
    ctx.prelude << "  " << kIdType << " " << cap << ";\n";
  }
  ctx.prelude << "};\n";
  ctx.prelude << "static VTable* " << cname << "_vtable = nullptr;\n";

  // Build call method into a temporary Ctx sharing class env for body.
  Ctx body_ctx;
  body_ctx.classes = ctx.classes;
  body_ctx.functions = ctx.functions;
  body_ctx.packages = ctx.packages;
  body_ctx.package_cpp = ctx.package_cpp;
  body_ctx.imports = ctx.imports;
  body_ctx.local_classes = ctx.local_classes;
  body_ctx.local_class_decls = ctx.local_class_decls;
  body_ctx.local_class_static_init = ctx.local_class_static_init;
  body_ctx.nested_emitted = ctx.nested_emitted;
  body_ctx.toplevel_vars = ctx.toplevel_vars;
  body_ctx.loaded_package_members = ctx.loaded_package_members;
  body_ctx.saw_load = ctx.saw_load;
  body_ctx.source_path = ctx.source_path;
  body_ctx.tmp = ctx.tmp;
  body_ctx.closure_id = ctx.closure_id;
  body_ctx.lambda_host = host;
  body_ctx.in_static_method = ctx.in_static_method;
  body_ctx.local_fn0 = ctx.local_fn0;
  ClassDecl fake;
  fake.name = cname;
  if (host) {
    Field hf;
    hf.name = "zefc_host";
    fake.fields.push_back(std::move(hf));
  }
  for (const std::string& cap : captures) {
    Field f;
    f.name = cap;
    fake.fields.push_back(std::move(f));
  }
  ClassInfo fake_info;
  fake_info.decl = &fake;
  if (host) {
    fake_info.field_names.insert("zefc_host");
  }
  for (const std::string& cap : captures) {
    fake_info.field_names.insert(cap);
  }
  for (const std::string& p : e.params) {
    fake_info.param_names.insert(p);
  }
  body_ctx.current_class = &fake_info;
  // Nested lambdas may capture outer params + already-captured env.
  body_ctx.env_params = ctx.env_params;
  body_ctx.local_vars.clear();
  for (const std::string& p : e.params) {
    body_ctx.env_params.push_back(p);
    body_ctx.local_vars.insert(p);
  }
  body_ctx.out << "static id\n" << cname << "__call_o(id self, int selector";
  for (size_t i = 0; i < e.params.size(); ++i) {
    body_ctx.out << ", id " << local_sym(e.params[i]);
  }
  body_ctx.out << ")\n{\n  (void)selector;\n";
  const std::string tmp = body_ctx.fresh("t");
  body_ctx.out << "  id " << tmp << ";\n";
  body_ctx.allow_return = true;
  body_ctx.import_binds = true;
  emit_body(body_ctx, e.body, tmp, false);
  body_ctx.out << "}\n\n";
  ctx.tmp = body_ctx.tmp;
  ctx.closure_id = body_ctx.closure_id;
  ctx.nested_emitted = body_ctx.nested_emitted;
  ctx.local_classes = body_ctx.local_classes;
  ctx.local_class_decls = body_ctx.local_class_decls;
  ctx.local_class_static_init = body_ctx.local_class_static_init;
  ctx.loaded_package_members = body_ctx.loaded_package_members;
  ctx.saw_load = ctx.saw_load || body_ctx.saw_load;
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
  if (host) {
    ctx.out << "    _c->zefc_host = self;\n";
  }
  for (const std::string& cap : captures) {
    if (is_field(ctx, cap)) {
      ctx.out << "    _c->" << cap << " = body<" << class_cpp(*ctx.current_class) << "_>(self)->"
              << cap << ";\n";
    } else {
      ctx.out << "    _c->" << cap << " = " << local_sym(cap) << ";\n";
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
    if (e.text == "null" || e.text == "false") {
      ctx.out << "  " << dst << " = null_id();\n";
      return;
    }
    if (e.text == "true") {
      ctx.out << "  " << dst << " = Int__from_i64(1);\n";
      return;
    }
    if (e.text == "this") {
      ctx.out << "  " << dst << " = self;\n";
      return;
    }
    if (!ctx.package_scope_cpp.empty() && ctx.package_scope_fields.count(e.text) &&
        !is_field(ctx, e.text) &&
        !(ctx.current_class && ctx.current_class->param_names.count(e.text))) {
      bool local = false;
      for (const std::string& p : ctx.env_params) {
        if (p == e.text) {
          local = true;
          break;
        }
      }
      if (!local) {
        // Package `my` / field visible in nested class methods and field inits.
        ctx.out << "  " << dst << " = g_" << ctx.package_scope_cpp << "." << e.text << ";\n";
        return;
      }
    }
    if (e.text == "super") {
      if (!ctx.current_class) {
        throw std::runtime_error("super outside subclass");
      }
      const bool dyn = ctx.current_class->dynamic_parent;
      if (ctx.current_class->decl->parent.empty() && !dyn) {
        // Implicit Object: bare `super` is a no-op (Zef Object has empty ctor).
        ctx.out << "  " << dst << " = self;\n";
        return;
      }
      // Bare `super` → parent zero-arg init. Mark before parent runs (Zef).
      emit_mark_super_inited(ctx);
      if (dyn) {
        const std::string cpp = class_cpp(*ctx.current_class);
        ctx.out << "  {\n";
        ctx.out << "    id _pcls = g_" << cpp << "_dyn_parent;\n";
        ctx.out << "    id _psup = ZEFC_SEND0(_pcls, " << sel_expr("call_o") << ");\n";
        ctx.out << "    body<" << cpp << "_>(self)->zefc_super = _psup;\n";
        ctx.out << "    " << dst << " = _psup;\n";
        ctx.out << "  }\n";
      } else {
        const std::string& parent = ctx.current_class->decl->parent;
        ctx.out << "  " << dst << " = " << parent << "__init(self, 0);\n";
      }
      return;
    }
    // Locals/params shadow fields and zero-arg getters (`my left` vs `fn left`).
    if (ctx.local_vars.count(e.text)) {
      if (ctx.local_fn0.count(e.text)) {
        emit_send(ctx, local_sym(e.text), mangle_method("call", 0), {}, dst);
      } else {
        ctx.out << "  " << dst << " = " << local_sym(e.text) << ";\n";
      }
      return;
    }
    // Instance-nested class name → class object bound to this outer instance.
    if (ctx.current_class && ctx.current_class->nested_instance.count(e.text)) {
      emit_bound_nested_class(ctx, ctx.current_class->nested_instance.at(e.text), "self", dst);
      return;
    }
    // Static field of current class (including from instance methods).
    if (ctx.current_class && ctx.current_class->decl) {
      bool static_field = false;
      for (const Field& f : ctx.current_class->decl->fields) {
        if (f.is_static && f.name == e.text) {
          static_field = true;
          break;
        }
      }
      if (static_field) {
        ctx.out << "  " << dst << " = g_" << class_cpp(*ctx.current_class) << "Class." << e.text
                << ";\n";
        return;
      }
    }
    if (is_field(ctx, e.text)) {
      const std::string& cls = class_cpp(*ctx.current_class);
      ctx.out << "  " << dst << " = body<" << cls << "_>(self)->" << e.text << ";\n";
    } else if (ctx.nest_captures.count(e.text)) {
      ctx.out << "  " << dst << " = body<" << ctx.nest_capture_type << ">(" << ctx.nest_capture_self
              << ")->" << e.text << ";\n";
    } else if (ctx.local_classes.count(e.text)) {
      // Nested class name → allocate class object with current captures.
      const std::vector<std::string>& caps = ctx.local_classes[e.text];
      ctx.out << "  {\n";
      ctx.out << "    ensure_" << e.text << "Class();\n";
      ctx.out << "    " << e.text << "Class_* _cls = alloc<" << e.text << "Class_>();\n";
      ctx.out << "    zefc_set_isa(_cls, " << e.text << "Class_vtable);\n";
      for (const std::string& cap : caps) {
        if (is_field(ctx, cap)) {
          ctx.out << "    _cls->" << cap << " = body<" << class_cpp(*ctx.current_class)
                  << "_>(self)->" << cap << ";\n";
        } else {
          ctx.out << "    _cls->" << cap << " = " << local_sym(cap) << ";\n";
        }
      }
      // Function-local `static fn ()` runs each time the class object is materialized.
      if (ctx.local_class_static_init.count(e.text)) {
        ctx.out << "    (void)" << e.text << "__static_init(as_id(_cls), 0);\n";
      }
      ctx.out << "    " << dst << " = as_id(_cls);\n";
      ctx.out << "  }\n";
    } else if (ctx.package_locals.count(e.text)) {
      ctx.out << "  " << dst << " = as_id(&g_" << ctx.package_locals[e.text] << "Class);\n";
    } else if (ctx.package_cpp.count(e.text)) {
      ctx.out << "  " << dst << " = as_id(&g_" << ctx.package_cpp[e.text] << ");\n";
    } else if (ctx.imports.count(e.text)) {
      const ImportBinding& b = ctx.imports[e.text];
      if (b.kind == ImportKind::Class) {
        ctx.out << "  " << dst << " = as_id(&g_" << b.cpp << "Class);\n";
      } else if (b.kind == ImportKind::Package) {
        ctx.out << "  " << dst << " = as_id(&g_" << b.cpp << ");\n";
      } else if (b.kind == ImportKind::Func && b.arity == 0) {
        emit_send(ctx, "as_id(&g_" + b.cpp + ")", mangle_method(b.member, 0), {}, dst);
      } else {
        ctx.out << "  " << dst << " = as_id(&g_" << b.cpp << ");\n";
      }
    } else if (ctx.current_class && ctx.current_class->static_methods0.count(e.text) &&
               !ctx.current_class->param_names.count(e.text)) {
      // Bare static method from instance or static context.
      if (ctx.in_static_method) {
        emit_send(ctx, "self", mangle_method(e.text, 0), {}, dst);
      } else {
        const std::string cpp = class_cpp(*ctx.current_class);
        emit_send(ctx, "as_id(&g_" + cpp + "Class)", mangle_method(e.text, 0), {}, dst);
      }
    } else if (ctx.lambda_host && ctx.lambda_host->static_methods0.count(e.text) &&
               !(ctx.current_class && ctx.current_class->param_names.count(e.text))) {
      emit_send(ctx, "body<" + class_cpp(*ctx.current_class) + "_>(self)->zefc_host",
                mangle_method(e.text, 0), {}, dst);
    } else if (ctx.lambda_host && ctx.lambda_host->method_names.count(e.text) &&
               !(ctx.current_class && ctx.current_class->param_names.count(e.text))) {
      emit_send(ctx, "body<" + class_cpp(*ctx.current_class) + "_>(self)->zefc_host",
                mangle_method(e.text, 0), {}, dst);
    } else if (ctx.current_class && ctx.current_class->method_names.count(e.text) &&
               !ctx.current_class->param_names.count(e.text)) {
      // Bare method name → self.method (e.g. `fn thingy stuff`)
      emit_send(ctx, "self", mangle_method(e.text, 0), {}, dst);
    } else if (ctx.functions.count(e.text) && ctx.functions[e.text].arity == 0) {
      // Bare zero-arg function name → call (println(foo), property getters).
      ctx.out << "  " << dst << " = fn_" << e.text << "();\n";
    } else if (try_resolve_enclosing(ctx, e.text, dst)) {
      return;
    } else {
      bool in_env = false;
      for (const std::string& p : ctx.env_params) {
        if (p == e.text) {
          in_env = true;
          break;
        }
      }
      if (!in_env) {
        for (const std::string& p : ctx.toplevel_vars) {
          if (p == e.text) {
            in_env = true;
            break;
          }
        }
      }
      if (in_env) {
        ctx.out << "  " << dst << " = " << local_sym(e.text) << ";\n";
      } else if ((e.text == "println" || e.text == "print") && !ctx.functions.count(e.text)) {
        // Zef: bare builtin println/print as a value is the integer 0 (test26d).
        ctx.out << "  " << dst << " = Int__from_i64(0);\n";
      } else if (ctx.current_class && !ctx.in_static_method && ctx.package_scope_cpp.empty() &&
                 !ctx.current_class->param_names.count(e.text) && ctx.current_class->decl &&
                 ctx.current_class->decl->name.compare(0, 7, "Closure") != 0 &&
                 ctx.classes.count(class_cpp(*ctx.current_class))) {
        // Dynamic: bare name → self.name (may be defined only on a subclass).
        // Only for real classes in ctx.classes (not package/closure fakes).
        emit_send(ctx, "self", mangle_method(e.text, 0), {}, dst);
      } else if (ctx.allow_unresolved) {
        ctx.out << "  zefc_error(\"cannot resolve get (call with no arguments) named " << e.text
                << "\");\n";
        ctx.out << "  " << dst << " = null_id();\n";
      } else {
        throw std::runtime_error("cannot resolve get (call with no arguments) named " + e.text);
      }
    }
    return;
  }
  case Expr::Kind::Lambda:
    emit_lambda(ctx, e, dst);
    return;
  case Expr::Kind::Block: {
    bool has_var = false;
    for (const BlockItem& item : e.body) {
      if (item.kind == BlockItem::Kind::VarDecl) {
        has_var = true;
        break;
      }
    }
    if (!has_var) {
      // Shares parent scope (import does not overwrite existing bindings).
      if (e.body.empty()) {
        ctx.out << "  " << dst << " = null_id();\n";
        return;
      }
      for (size_t i = 0; i + 1 < e.body.size(); ++i) {
        emit_block_item(ctx, e.body[i], nullptr);
      }
      emit_block_item(ctx, e.body.back(), &dst);
      return;
    }
    // New scope: shadow outer bindings, then import may overwrite.
    ctx.out << "  {\n";
    const bool saved_import_binds = ctx.import_binds;
    const auto saved_locals = ctx.local_vars;
    const auto saved_env = ctx.env_params;
    std::unordered_set<std::string> declared;
    for (const BlockItem& item : e.body) {
      if (item.kind == BlockItem::Kind::VarDecl) {
        declared.insert(item.var_name);
      }
    }
    std::unordered_set<std::string> copied;
    auto copy_name = [&](const std::string& name) {
      if (declared.count(name) || copied.count(name)) {
        return;
      }
      copied.insert(name);
      const std::string snap = ctx.fresh("snap");
      if (ctx.nest_captures.count(name)) {
        ctx.out << "  id " << snap << " = body<" << ctx.nest_capture_type << ">("
                << ctx.nest_capture_self << ")->" << name << ";\n";
      } else if (is_field(ctx, name)) {
        ctx.out << "  id " << snap << " = body<" << class_cpp(*ctx.current_class)
                << "_>(self)->" << name << ";\n";
      } else {
        ctx.out << "  id " << snap << " = " << local_sym(name) << ";\n";
      }
      ctx.out << "  id " << local_sym(name) << " = " << snap << ";\n";
      ctx.local_vars.insert(name);
    };
    for (const std::string& name : ctx.env_params) {
      copy_name(name);
    }
    for (const std::string& name : ctx.toplevel_vars) {
      copy_name(name);
    }
    ctx.import_binds = true;
    if (e.body.empty()) {
      ctx.out << "  " << dst << " = null_id();\n";
    } else {
      for (size_t i = 0; i + 1 < e.body.size(); ++i) {
        emit_block_item(ctx, e.body[i], nullptr);
      }
      emit_block_item(ctx, e.body.back(), &dst);
    }
    ctx.import_binds = saved_import_binds;
    ctx.local_vars = saved_locals;
    ctx.env_params = saved_env;
    ctx.out << "  }\n";
    return;
  }
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
  case Expr::Kind::Number: {
    const bool is_float =
        e.text.find('.') != std::string::npos || e.text.find('e') != std::string::npos ||
        e.text.find('E') != std::string::npos;
    if (is_float) {
      ctx.out << "  " << dst << " = Double__from_f64(" << e.text << ");\n";
    } else if (e.text.size() >= 2 && e.text[0] == '0' &&
               (e.text[1] == 'x' || e.text[1] == 'X')) {
      ctx.out << "  " << dst << " = Int__from_i64(static_cast<long long>(" << e.text
              << "ULL));\n";
    } else {
      ctx.out << "  " << dst << " = Int__from_i64(" << e.text << ");\n";
    }
    return;
  }
  case Expr::Kind::RootPackage:
    throw std::runtime_error("`..` must be followed by a member name");
  case Expr::Kind::Dot: {
    // ..name → root-package member (package / class object)
    if (e.lhs && e.lhs->kind == Expr::Kind::RootPackage) {
      if (ctx.package_cpp.count(e.text)) {
        ctx.out << "  " << dst << " = as_id(&g_" << ctx.package_cpp[e.text] << ");\n";
        return;
      }
      if (ctx.classes.count(e.text)) {
        const std::string cpp = class_cpp(ctx.classes[e.text]);
        ctx.out << "  " << dst << " = as_id(&g_" << cpp << "Class);\n";
        return;
      }
      throw std::runtime_error("unknown root package member `" + e.text + "`");
    }
    if (e.lhs && e.lhs->kind == Expr::Kind::Ident && is_builtin_name(e.lhs->text)) {
      // e.g. println.call(...) — Zef reports this as a bad int method.
      throw std::runtime_error("bad method for int");
    }
    // super.method → direct superclass method call
    if (e.lhs && e.lhs->kind == Expr::Kind::Ident && e.lhs->text == "super") {
      if (!ctx.current_class ||
          (ctx.current_class->decl->parent.empty() && !ctx.current_class->dynamic_parent)) {
        throw std::runtime_error("super.method outside subclass");
      }
      const std::string mangled = mangle_method(e.text, 0);
      if (ctx.current_class->dynamic_parent) {
        const std::string cpp = class_cpp(*ctx.current_class);
        ctx.out << "  " << dst << " = ZEFC_SEND0(body<" << cpp << "_>(self)->zefc_super, "
                << sel_expr(mangled) << ");\n";
      } else {
        const std::string& parent = ctx.current_class->decl->parent;
        ctx.out << "  " << dst << " = " << parent << "__" << mangled << "(self, "
                << sel_expr(mangled) << ");\n";
      }
      return;
    }
    // ClassName.member → static field, nested static class, or zero-arg static method
    if (e.lhs && e.lhs->kind == Expr::Kind::Ident && ctx.classes.count(e.lhs->text)) {
      const std::string& cls = e.lhs->text;
      const ClassInfo& ci = ctx.classes[cls];
      const std::string cpp = class_cpp(ci);
      check_private_class_member(ctx, ci, e.text);
      ctx.out << "  ensure_" << cpp << "();\n";
      if (ci.nested_instance.count(e.text)) {
        throw std::runtime_error("no such method: " + e.text);
      }
      if (ci.static_methods0.count(e.text)) {
        ctx.out << "  " << dst << " = " << method_sym(cpp, true, mangle_method(e.text, 0))
                << "(as_id(&g_" << cpp << "Class), " << sel_expr(mangle_method(e.text, 0))
                << ");\n";
      } else if (ci.methods.count(e.text) || ci.method_names.count(e.text)) {
        // Instance method is not a class-object member (private3e).
        throw std::runtime_error("no such method: " + e.text);
      } else {
        ctx.out << "  " << dst << " = g_" << cpp << "Class." << e.text << ";\n";
      }
      return;
    }
    // package.member → method / class / nested package (via send getters or methods)
    if (e.lhs && e.lhs->kind == Expr::Kind::Ident && ctx.package_cpp.count(e.lhs->text)) {
      const std::string& pcpp = ctx.package_cpp[e.lhs->text];
      const PackageInfo& pkg = ctx.packages[pcpp];
      if (pkg.funcs.count(e.text)) {
        emit_send(ctx, "as_id(&g_" + pcpp + ")",
                  mangle_method(e.text, pkg.funcs.at(e.text)->params.size()), {}, dst);
      } else {
        emit_send(ctx, "as_id(&g_" + pcpp + ")", mangle_getter(e.text), {}, dst);
      }
      return;
    }
    // After load(...), unknown package.member resolves via runtime slots.
    if (ctx.saw_load && e.lhs && e.lhs->kind == Expr::Kind::Ident &&
        !ctx.classes.count(e.lhs->text) && !ctx.package_cpp.count(e.lhs->text)) {
      ctx.out << "  " << dst << " = package_slot_get(\"" << e.lhs->text << "\", \"" << e.text
              << "\");\n";
      return;
    }
    check_private_member_access(ctx, e.text);
    const std::string recv = ctx.fresh("t");
    ctx.out << "  id " << recv << ";\n";
    emit_expr(ctx, *e.lhs, recv);
    emit_send(ctx, recv, mangle_method(e.text, 0), {}, dst);
    return;
  }
  case Expr::Kind::Binary: {
    // Short-circuit || before evaluating rhs.
    if (e.text == "||") {
      emit_expr(ctx, *e.lhs, dst);
      ctx.out << "  if (!" << truthy_expr(dst) << ") {\n";
      emit_expr(ctx, *e.rhs, dst);
      ctx.out << "  }\n";
      return;
    }
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
      // Ints/doubles: numeric. Objects: equal method, else identity (Object.equal).
      ctx.out << "  if (id_is_double(" << a << ") || (id_is_int(" << a << ") && id_is_int("
              << b << "))) {\n";
      ctx.out << "    " << dst << " = Int__from_i64((id_is_double(" << a
              << ") ? (Double__to_f64(" << a << ") == Double__to_f64(" << b
              << ")) : (Int__to_i64(" << a << ") == Int__to_i64(" << b << "))) ? 1 : 0);\n";
      ctx.out << "  } else {\n";
      ctx.out << "    zefc_method _eqm = zefc_method_at(" << a << ", " << sel_expr("equal_o")
              << ");\n";
      ctx.out << "    if (_eqm == doesNotUnderstand) {\n";
      ctx.out << "      " << dst << " = Int__from_i64(" << a << " == " << b << " ? 1 : 0);\n";
      ctx.out << "    } else {\n";
      ctx.out << "      " << dst << " = _eqm(" << a << ", " << sel_expr("equal_o") << ", " << b
              << ");\n";
      ctx.out << "    }\n";
      ctx.out << "  }\n";
    } else if (op == "!=") {
      ctx.out << "  if (id_is_double(" << a << ") || (id_is_int(" << a << ") && id_is_int("
              << b << "))) {\n";
      ctx.out << "    " << dst << " = Int__from_i64((id_is_double(" << a
              << ") ? (Double__to_f64(" << a << ") != Double__to_f64(" << b
              << ")) : (Int__to_i64(" << a << ") != Int__to_i64(" << b << "))) ? 1 : 0);\n";
      ctx.out << "  } else {\n";
      const std::string eq = ctx.fresh("t");
      ctx.out << "    id " << eq << ";\n";
      ctx.out << "    zefc_method _eqm = zefc_method_at(" << a << ", " << sel_expr("equal_o")
              << ");\n";
      ctx.out << "    if (_eqm == doesNotUnderstand) {\n";
      ctx.out << "      " << eq << " = Int__from_i64(" << a << " == " << b << " ? 1 : 0);\n";
      ctx.out << "    } else {\n";
      ctx.out << "      " << eq << " = _eqm(" << a << ", " << sel_expr("equal_o") << ", " << b
              << ");\n";
      ctx.out << "    }\n";
      ctx.out << "    " << dst << " = Int__from_i64(" << truthy_expr(eq) << " ? 0 : 1);\n";
      ctx.out << "  }\n";
    } else if (op == "<") {
      ctx.out << "  " << dst << " = Int__from_i64((id_is_double(" << a
              << ") ? (Double__to_f64(" << a << ") < Double__to_f64(" << b
              << ")) : (Int__to_i64(" << a << ") < Int__to_i64(" << b << "))) ? 1 : 0);\n";
    } else if (op == "<=") {
      ctx.out << "  " << dst << " = Int__from_i64((id_is_double(" << a
              << ") ? (Double__to_f64(" << a << ") <= Double__to_f64(" << b
              << ")) : (Int__to_i64(" << a << ") <= Int__to_i64(" << b << "))) ? 1 : 0);\n";
    } else if (op == "&") {
      ctx.out << "  " << dst << " = Int__from_i64(Int__to_i64(" << a << ") & Int__to_i64(" << b
              << "));\n";
    } else if (op == "|") {
      ctx.out << "  " << dst << " = Int__from_i64(Int__to_i64(" << a << ") | Int__to_i64(" << b
              << "));\n";
    } else if (op == "^") {
      ctx.out << "  " << dst << " = Int__from_i64(Int__to_i64(" << a << ") ^ Int__to_i64(" << b
              << "));\n";
    } else if (op == "<<") {
      ctx.out << "  " << dst << " = Int__from_i64(Int__to_i64(" << a << ") << (Int__to_i64("
              << b << ") & 63));\n";
    } else if (op == ">>") {
      ctx.out << "  " << dst << " = Int__from_i64(Int__to_i64(" << a << ") >> (Int__to_i64("
              << b << ") & 63));\n";
    } else if (op == ">>>") {
      ctx.out << "  " << dst << " = Int__from_i64(static_cast<long long>("
              << "static_cast<unsigned long long>(Int__to_i64(" << a
              << ")) >> (Int__to_i64(" << b << ") & 63)));\n";
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
    ctx.out << "  if (!" << truthy_expr(cond) << ") {\n    break;\n  }\n";
    ++ctx.loop_depth;
    for (const BlockItem& item : e.body) {
      emit_block_item(ctx, item, nullptr);
    }
    --ctx.loop_depth;
    ctx.out << "  }\n";
    ctx.out << "  " << dst << " = null_id();\n";
    return;
  }
  case Expr::Kind::If: {
    const std::string cond = ctx.fresh("t");
    ctx.out << "  id " << cond << ";\n";
    emit_expr(ctx, *e.lhs, cond);
    ctx.out << "  if (" << truthy_expr(cond) << ") {\n";
    if (e.body.empty()) {
      ctx.out << "  " << dst << " = null_id();\n";
    } else {
      for (size_t i = 0; i + 1 < e.body.size(); ++i) {
        emit_block_item(ctx, e.body[i], nullptr);
      }
      emit_block_item(ctx, e.body.back(), &dst);
    }
    ctx.out << "  } else {\n";
    if (e.else_body.empty()) {
      ctx.out << "  " << dst << " = null_id();\n";
    } else {
      for (size_t i = 0; i + 1 < e.else_body.size(); ++i) {
        emit_block_item(ctx, e.else_body[i], nullptr);
      }
      emit_block_item(ctx, e.else_body.back(), &dst);
    }
    ctx.out << "  }\n";
    return;
  }
  case Expr::Kind::Break:
    if (ctx.loop_depth <= 0) {
      throw std::runtime_error("cannot break");
    }
    ctx.out << "  break;\n";
    ctx.out << "  " << dst << " = null_id();\n";
    return;
  case Expr::Kind::Continue:
    if (ctx.loop_depth <= 0) {
      throw std::runtime_error("cannot continue");
    }
    ctx.out << "  continue;\n";
    ctx.out << "  " << dst << " = null_id();\n";
    return;
  case Expr::Kind::Return: {
    if (!ctx.allow_return) {
      throw std::runtime_error("cannot return");
    }
    if (e.rhs) {
      const std::string v = ctx.fresh("t");
      ctx.out << "  id " << v << ";\n";
      emit_expr(ctx, *e.rhs, v);
      ctx.out << "  return " << v << ";\n";
    } else {
      // Bare `return` yields 0 (Zef: DoReturn(IntLiteral(0))).
      ctx.out << "  return Int__from_i64(0);\n";
    }
    ctx.out << "  " << dst << " = null_id();\n";
    return;
  }
  case Expr::Kind::Unary: {
    const std::string a = ctx.fresh("t");
    ctx.out << "  id " << a << ";\n";
    emit_expr(ctx, *e.rhs, a);
    if (e.text == "-") {
      ctx.out << "  if (id_is_double(" << a << ")) {\n";
      ctx.out << "    " << dst << " = Double__from_f64(-Double__to_f64(" << a << "));\n";
      ctx.out << "  } else {\n";
      const std::string z = ctx.fresh("t");
      ctx.out << "    id " << z << " = Int__from_i64(0);\n";
      ctx.out << "    " << dst << " = ZEFC_SEND1(" << z << ", ZEFC_SEL_sub_o, " << a << ");\n";
      ctx.out << "  }\n";
    } else if (e.text == "~") {
      ctx.out << "  " << dst << " = Int__from_i64(~Int__to_i64(" << a << "));\n";
    } else if (e.text == "!") {
      // Null / int 0 are falsy (Zef); tagged int0 is a non-null odd pointer.
      ctx.out << "  " << dst << " = Int__from_i64((!(" << a << ") || (id_is_int32(" << a
              << ") && Int__to_i64(" << a << ") == 0)) ? 1 : 0);\n";
    } else {
      throw std::runtime_error("unsupported unary op: " + e.text);
    }
    return;
  }
  case Expr::Kind::Assign: {
    const bool plus_eq = (e.text == "+=");
    const bool minus_eq = (e.text == "-=");
    const bool star_eq = (e.text == "*=");
    const bool bit_eq =
        (e.text == "^=" || e.text == "&=" || e.text == "|=");
    const bool is_rmw = plus_eq || minus_eq || star_eq || bit_eq;
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
    if (!is_rmw && e.lhs && e.lhs->kind == Expr::Kind::Index) {
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

    if (is_rmw) {
      // lhs OP= rhs  →  lhs = lhs OP rhs
      const std::string cur = ctx.fresh("t");
      const std::string other = ctx.fresh("t");
      ctx.out << "  id " << cur << ";\n  id " << other << ";\n";
      if (e.lhs && e.lhs->kind == Expr::Kind::Ident && is_field(ctx, e.lhs->text)) {
        const std::string& cls = class_cpp(*ctx.current_class);
        ctx.out << "  " << cur << " = body<" << cls << "_>(self)->" << e.lhs->text << ";\n";
      } else if (e.lhs && e.lhs->kind == Expr::Kind::Ident) {
        ctx.out << "  " << cur << " = " << local_sym(e.lhs->text) << ";\n";
      } else if (e.lhs && e.lhs->kind == Expr::Kind::Dot) {
        emit_expr(ctx, *e.lhs, cur);
      } else if (e.lhs && e.lhs->kind == Expr::Kind::Index) {
        emit_expr(ctx, *e.lhs, cur);
      } else {
        throw std::runtime_error(e.text + " target must be an identifier, field, or index");
      }
      // Uninitialized / null behaves as 0 for numeric RMW (Zef `my g` then `g += 1`).
      if (plus_eq || minus_eq || star_eq || e.text == "^=" || e.text == "&=" || e.text == "|=") {
        ctx.out << "  if (!" << cur << ") {\n    " << cur << " = Int__from_i64(0);\n  }\n";
      }
      emit_expr(ctx, *e.rhs, other);
      if (plus_eq) {
        ctx.out << "  " << rhs << " = ZEFC_SEND1(" << cur << ", ZEFC_SEL_add_o, " << other
                << ");\n";
      } else if (minus_eq) {
        ctx.out << "  " << rhs << " = ZEFC_SEND1(" << cur << ", ZEFC_SEL_sub_o, " << other
                << ");\n";
      } else if (star_eq) {
        ctx.out << "  " << rhs << " = ZEFC_SEND1(" << cur << ", ZEFC_SEL_mul_o, " << other
                << ");\n";
      } else if (e.text == "^=") {
        ctx.out << "  " << rhs << " = Int__from_i64(Int__to_i64(" << cur << ") ^ Int__to_i64("
                << other << "));\n";
      } else if (e.text == "&=") {
        ctx.out << "  " << rhs << " = Int__from_i64(Int__to_i64(" << cur << ") & Int__to_i64("
                << other << "));\n";
      } else if (e.text == "|=") {
        ctx.out << "  " << rhs << " = Int__from_i64(Int__to_i64(" << cur << ") | Int__to_i64("
                << other << "));\n";
      }
    } else {
      emit_expr(ctx, *e.rhs, rhs);
    }
    if (e.lhs && e.lhs->kind == Expr::Kind::Dot) {
      // ClassName.field = rhs (static)
      if (e.lhs->lhs && e.lhs->lhs->kind == Expr::Kind::Ident &&
          ctx.classes.count(e.lhs->lhs->text)) {
        ctx.out << "  g_" << class_cpp(ctx.classes[e.lhs->lhs->text]) << "Class." << e.lhs->text
                << " = " << rhs << ";\n";
        ctx.out << "  " << dst << " = " << rhs << ";\n";
        return;
      }
      // obj.field = rhs → setter send
      {
        // Private accessible fields report the setter name (accessors2d).
        bool private_elsewhere = false;
        bool allowed_here = false;
        for (const auto& kv : ctx.classes) {
          if (!kv.second.private_fields.count(e.lhs->text)) {
            continue;
          }
          if (ctx.current_class && class_cpp(*ctx.current_class) == class_cpp(kv.second)) {
            allowed_here = true;
          } else {
            private_elsewhere = true;
          }
        }
        if (private_elsewhere && !allowed_here) {
          throw std::runtime_error("no such method: set_" + e.lhs->text);
        }
      }
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
    if (ctx.current_class && ctx.current_class->decl) {
      bool static_field = false;
      for (const Field& f : ctx.current_class->decl->fields) {
        if (f.is_static && f.name == name) {
          static_field = true;
          break;
        }
      }
      if (static_field) {
        ctx.out << "  g_" << class_cpp(*ctx.current_class) << "Class." << name << " = " << rhs
                << ";\n";
        ctx.out << "  " << dst << " = " << rhs << ";\n";
        return;
      }
    }
    if (is_field(ctx, name)) {
      const std::string& cls = class_cpp(*ctx.current_class);
      ctx.out << "  body<" << cls << "_>(self)->" << name << " = " << rhs << ";\n";
      ctx.out << "  " << dst << " = " << rhs << ";\n";
    } else if (ctx.functions.count("set_" + name)) {
      // Top-level property: x = v → set_x(v)
      ctx.out << "  " << dst << " = fn_set_" << name << "(" << rhs << ");\n";
    } else {
      ctx.out << "  " << local_sym(name) << " = " << rhs << ";\n";
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
      // ..name(...) → root-package member call (same builtins / globals as bare name)
      if (e.lhs->lhs && e.lhs->lhs->kind == Expr::Kind::RootPackage) {
        const std::string& callee = e.lhs->text;
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
        if (ctx.classes.count(callee)) {
          const ClassInfo& ci = ctx.classes[callee];
          const std::string cpp = class_cpp(ci);
          check_ctor_visible(ctx, ci);
          if (arg_tmps.empty() && ci.has_static_call0) {
            ctx.out << "  ensure_" << cpp << "();\n";
            const char* call_sym =
                ci.synth_nested_call ? "__call_o" : "Class__call_o";
            ctx.out << "  " << dst << " = " << cpp << call_sym << "(as_id(&g_" << cpp
                    << "Class), " << sel_expr("call_o") << ");\n";
          } else {
            ctx.out << "  " << dst << " = " << cpp << "__new(null_id(), 0";
            for (const std::string& a : arg_tmps) {
              ctx.out << ", " << a;
            }
            ctx.out << ");\n";
          }
          return;
        }
        if (ctx.package_cpp.count(callee)) {
          throw std::runtime_error("root package member `" + callee + "` is not callable");
        }
        throw std::runtime_error("unknown root package member `" + callee + "`");
      }
      if (e.lhs->lhs && e.lhs->lhs->kind == Expr::Kind::Ident && e.lhs->lhs->text == "super") {
        if (!ctx.current_class ||
            (ctx.current_class->decl->parent.empty() && !ctx.current_class->dynamic_parent)) {
          throw std::runtime_error("super.method outside subclass");
        }
        const std::string mangled = mangle_method(e.lhs->text, e.args.size());
        if (ctx.current_class->dynamic_parent) {
          const std::string cpp = class_cpp(*ctx.current_class);
          emit_send(ctx, "body<" + cpp + "_>(self)->zefc_super", mangled, arg_tmps, dst);
        } else {
          const std::string& parent = ctx.current_class->decl->parent;
          ctx.out << "  " << dst << " = " << parent << "__" << mangled << "(self, "
                  << sel_expr(mangled);
          for (const std::string& a : arg_tmps) {
            ctx.out << ", " << a;
          }
          ctx.out << ");\n";
        }
        return;
      }
      // ClassName.member(args): static method, or nested static class / field → call_o
      if (e.lhs->lhs && e.lhs->lhs->kind == Expr::Kind::Ident &&
          ctx.classes.count(e.lhs->lhs->text)) {
        const ClassInfo& ci = ctx.classes[e.lhs->lhs->text];
        const std::string cpp = class_cpp(ci);
        if (ci.nested_instance.count(e.lhs->text)) {
          throw std::runtime_error("no such method: " + e.lhs->text);
        }
        if (ci.decl) {
          for (const Method& m : ci.decl->methods) {
            if (m.is_static && m.name == e.lhs->text && m.params.size() == arg_tmps.size()) {
              check_private_class_member(ctx, ci, e.lhs->text);
              ctx.out << "  ensure_" << cpp << "();\n";
              ctx.out << "  " << dst << " = "
                      << method_sym(cpp, true, mangle_method(e.lhs->text, arg_tmps.size()))
                      << "(as_id(&g_" << cpp << "Class), "
                      << sel_expr(mangle_method(e.lhs->text, arg_tmps.size()));
              for (const std::string& a : arg_tmps) {
                ctx.out << ", " << a;
              }
              ctx.out << ");\n";
              return;
            }
          }
        }
        const std::string recv = ctx.fresh("t");
        ctx.out << "  ensure_" << cpp << "();\n";
        ctx.out << "  id " << recv << " = g_" << cpp << "Class." << e.lhs->text << ";\n";
        emit_send(ctx, recv, "call_o", arg_tmps, dst);
        return;
      }
      // package.member(args)
      if (e.lhs->lhs && e.lhs->lhs->kind == Expr::Kind::Ident &&
          ctx.package_cpp.count(e.lhs->lhs->text)) {
        const std::string& pcpp = ctx.package_cpp[e.lhs->lhs->text];
        const PackageInfo& pkg = ctx.packages[pcpp];
        if (pkg.funcs.count(e.lhs->text)) {
          emit_send(ctx, "as_id(&g_" + pcpp + ")",
                    mangle_method(e.lhs->text, arg_tmps.size()), arg_tmps, dst);
          return;
        }
        if (pkg.classes.count(e.lhs->text)) {
          const std::string& ccpp = pkg.classes.at(e.lhs->text);
          const ClassInfo& ci = ctx.classes.at(ccpp);
          if (arg_tmps.empty() && ci.has_static_call0) {
            const std::string prop = ctx.fresh("t");
            ctx.out << "  id " << prop << ";\n";
            emit_send(ctx, "as_id(&g_" + pcpp + ")", mangle_getter(e.lhs->text), {}, prop);
            emit_send(ctx, prop, "call_o", arg_tmps, dst);
          } else {
            ctx.out << "  " << dst << " = " << ccpp << "__new(null_id(), 0";
            for (const std::string& a : arg_tmps) {
              ctx.out << ", " << a;
            }
            ctx.out << ");\n";
          }
          return;
        }
        // Nested package field → get then call_o
        const std::string prop = ctx.fresh("t");
        ctx.out << "  id " << prop << ";\n";
        emit_send(ctx, "as_id(&g_" + pcpp + ")", mangle_getter(e.lhs->text), {}, prop);
        emit_send(ctx, prop, "call_o", arg_tmps, dst);
        return;
      }
      if (e.lhs->lhs && e.lhs->lhs->kind == Expr::Kind::Ident &&
          is_builtin_name(e.lhs->lhs->text)) {
        throw std::runtime_error("bad method for int");
      }
      const std::string recv = ctx.fresh("t");
      ctx.out << "  id " << recv << ";\n";
      emit_expr(ctx, *e.lhs->lhs, recv);
      // Static nested class is not an instance member (o.Bar when Bar is static).
      for (const auto& kv : ctx.classes) {
        if (kv.second.nested_static.count(e.lhs->text) &&
            !kv.second.nested_instance.count(e.lhs->text)) {
          throw std::runtime_error("no such method: " + e.lhs->text);
        }
      }
      // Nested instance class: recv.Bar() → get Bar then call_o (not zero-arg method Bar).
      bool treat_as_get_call = false;
      for (const auto& kv : ctx.classes) {
        if (kv.second.nested_instance.count(e.lhs->text)) {
          treat_as_get_call = true;
          break;
        }
      }
      // Package class member accessed via nested recv (foo.bar is package, etc.)
      for (const auto& kv : ctx.packages) {
        if (kv.second.classes.count(e.lhs->text)) {
          treat_as_get_call = true;
          break;
        }
      }
      if (treat_as_get_call) {
        check_private_member_access(ctx, e.lhs->text);
        const std::string prop = ctx.fresh("t");
        ctx.out << "  id " << prop << ";\n";
        emit_send(ctx, recv, mangle_method(e.lhs->text, 0), {}, prop);
        emit_send(ctx, prop, "call_o", arg_tmps, dst);
      } else {
        emit_send(ctx, recv, mangle_method(e.lhs->text, e.args.size()), arg_tmps, dst);
      }
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
        if (!ctx.current_class) {
          throw std::runtime_error("super(...) outside subclass");
        }
        const bool dyn = ctx.current_class->dynamic_parent;
        if (ctx.current_class->decl->parent.empty() && !dyn) {
          // Implicit Object: super(...) is a no-op.
          ctx.out << "  " << dst << " = self;\n";
          return;
        }
        emit_mark_super_inited(ctx);
        if (dyn) {
          const std::string cpp = class_cpp(*ctx.current_class);
          ctx.out << "  {\n";
          ctx.out << "    id _pcls = g_" << cpp << "_dyn_parent;\n";
          ctx.out << "    id _psup;\n";
          emit_send(ctx, "_pcls", "call_o", arg_tmps, "_psup");
          ctx.out << "    body<" << cpp << "_>(self)->zefc_super = _psup;\n";
          ctx.out << "    " << dst << " = _psup;\n";
          ctx.out << "  }\n";
        } else {
          const std::string& parent = ctx.current_class->decl->parent;
          ctx.out << "  " << dst << " = " << parent << "__init(self, 0";
          for (const std::string& a : arg_tmps) {
            ctx.out << ", " << a;
          }
          ctx.out << ");\n";
        }
        return;
      }
      if (callee == "println" || callee == "print") {
        // User-defined fn println/print shadows the builtin (testd); `..println` is root.
        if (!ctx.functions.count(callee)) {
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
      if (callee == "Array") {
        if (arg_tmps.empty()) {
          ctx.out << "  " << dst << " = Array__new();\n";
        } else if (arg_tmps.size() == 1) {
          ctx.out << "  " << dst << " = Array__with_size(static_cast<int>(Int__to_i64("
                  << arg_tmps[0] << ")));\n";
        } else {
          throw std::runtime_error("Array(...) expects 0 or 1 argument");
        }
        return;
      }
      if (callee == "int") {
        if (arg_tmps.size() != 1) {
          throw std::runtime_error("int(...) expects 1 argument");
        }
        const std::string& a = arg_tmps[0];
        ctx.out << "  if (id_is_double(" << a << ")) {\n";
        ctx.out << "    " << dst << " = Int__from_i64(static_cast<long long>(Double__to_f64(" << a
                << ")));\n";
        ctx.out << "  } else if (id_is_int(" << a << ")) {\n";
        ctx.out << "    " << dst << " = " << a << ";\n";
        ctx.out << "  } else {\n";
        ctx.out << "    " << dst << " = Int__from_i64(std::atoll(String__cstr(ZEFC_SEND0(" << a
                << ", ZEFC_SEL_toString_o))));\n";
        ctx.out << "  }\n";
        return;
      }
      if (callee == "double") {
        if (arg_tmps.size() != 1) {
          throw std::runtime_error("double(...) expects 1 argument");
        }
        const std::string& a = arg_tmps[0];
        ctx.out << "  if (id_is_int(" << a << ")) {\n";
        ctx.out << "    " << dst << " = Double__from_f64(static_cast<double>(Int__to_i64(" << a
                << ")));\n";
        ctx.out << "  } else if (id_is_double(" << a << ")) {\n";
        ctx.out << "    " << dst << " = " << a << ";\n";
        ctx.out << "  } else {\n";
        ctx.out << "    " << dst << " = Double__from_f64(std::atof(String__cstr(ZEFC_SEND0(" << a
                << ", ZEFC_SEL_toString_o))));\n";
        ctx.out << "  }\n";
        return;
      }
      if (callee == "error") {
        if (arg_tmps.empty()) {
          throw std::runtime_error("error expects at least 1 argument");
        }
        if (arg_tmps.size() == 1) {
          ctx.out << "  zefc_error(String__cstr(ZEFC_SEND0(" << arg_tmps[0]
                  << ", ZEFC_SEL_toString_o)));\n";
        } else {
          const std::string acc = ctx.fresh("t");
          ctx.out << "  id " << acc << " = String__from_utf8(\"\");\n";
          for (const std::string& a : arg_tmps) {
            const std::string s = ctx.fresh("t");
            ctx.out << "  id " << s << " = ZEFC_SEND0(" << a << ", ZEFC_SEL_toString_o);\n";
            ctx.out << "  " << acc << " = ZEFC_SEND1(" << acc << ", ZEFC_SEL_add_o, " << s
                    << ");\n";
          }
          ctx.out << "  zefc_error(String__cstr(" << acc << "));\n";
        }
        ctx.out << "  " << dst << " = null_id();\n";
        return;
      }
      if (callee == "load") {
        if (arg_tmps.size() != 1) {
          throw std::runtime_error("load expects 1 argument");
        }
        // Runtime module map (pre-registered / dlopen later); not source parse.
        ctx.saw_load = true;
        if (!e.args.empty() && e.args[0] && e.args[0]->kind == Expr::Kind::String) {
          note_load_exports(ctx, e.args[0]->text);
        }
        ctx.out << "  module_load(String__cstr(" << arg_tmps[0] << "));\n";
        ctx.out << "  " << dst << " = null_id();\n";
        return;
      }
      if (ctx.current_class && ctx.current_class->methods.count(callee) &&
          ctx.current_class->methods.at(callee) == arg_tmps.size() &&
          !ctx.current_class->param_names.count(callee)) {
        emit_send(ctx, "self", mangle_method(callee, arg_tmps.size()), arg_tmps, dst);
        return;
      }
      // Bare static method with args (deltablue2: weaker(s1, s2) inside static method).
      if (ctx.current_class && ctx.current_class->decl &&
          !ctx.current_class->param_names.count(callee)) {
        for (const Method& m : ctx.current_class->decl->methods) {
          if (m.is_static && m.name == callee && m.params.size() == arg_tmps.size()) {
            if (ctx.in_static_method) {
              emit_send(ctx, "self", mangle_method(callee, arg_tmps.size()), arg_tmps, dst);
            } else {
              const std::string cpp = class_cpp(*ctx.current_class);
              emit_send(ctx, "as_id(&g_" + cpp + "Class)",
                        mangle_method(callee, arg_tmps.size()), arg_tmps, dst);
            }
            return;
          }
        }
      }
      if (ctx.package_methods.count(callee)) {
        emit_send(ctx, "self", mangle_method(callee, arg_tmps.size()), arg_tmps, dst);
        return;
      }
      if (ctx.package_locals.count(callee) ||
          (ctx.imports.count(callee) && ctx.imports[callee].kind == ImportKind::Class)) {
        const std::string& cpp = ctx.package_locals.count(callee)
                                   ? ctx.package_locals[callee]
                                   : ctx.imports[callee].cpp;
        const ClassInfo& ci = ctx.classes.at(cpp);
        if (arg_tmps.empty() && ci.has_static_call0) {
          ctx.out << "  ensure_" << cpp << "();\n";
          const char* call_sym = ci.synth_nested_call ? "__call_o" : "Class__call_o";
          ctx.out << "  " << dst << " = " << cpp << call_sym << "(as_id(&g_" << cpp
                  << "Class), " << sel_expr("call_o") << ");\n";
        } else {
          ctx.out << "  " << dst << " = " << cpp << "__new(null_id(), 0";
          for (const std::string& a : arg_tmps) {
            ctx.out << ", " << a;
          }
          ctx.out << ");\n";
        }
        return;
      }
      if (ctx.imports.count(callee) && ctx.imports[callee].kind == ImportKind::Func) {
        const ImportBinding& b = ctx.imports[callee];
        emit_send(ctx, "as_id(&g_" + b.cpp + ")", mangle_method(b.member, arg_tmps.size()),
                  arg_tmps, dst);
        return;
      }
      if (ctx.classes.count(callee)) {
        const ClassInfo& ci = ctx.classes[callee];
        const std::string cpp = class_cpp(ci);
        check_ctor_visible(ctx, ci);
        // Class() with static call and no ctor args → invoke class.call (Zef staticcall).
        if (arg_tmps.empty() && ci.has_static_call0) {
          ctx.out << "  ensure_" << cpp << "();\n";
          const char* call_sym = ci.synth_nested_call ? "__call_o" : "Class__call_o";
          ctx.out << "  " << dst << " = " << cpp << call_sym << "(as_id(&g_" << cpp
                  << "Class), " << sel_expr("call_o") << ");\n";
        } else {
          ctx.out << "  " << dst << " = " << cpp << "__new(null_id(), 0";
          for (const std::string& a : arg_tmps) {
            ctx.out << ", " << a;
          }
          ctx.out << ");\n";
        }
        return;
      }
      if (ctx.functions.count(callee)) {
        // Nullary println/print override ignores call arguments (testd).
        if ((callee == "println" || callee == "print") &&
            ctx.functions[callee].arity == 0) {
          ctx.out << "  " << dst << " = fn_" << callee << "();\n";
          return;
        }
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
      // Dynamic instance method with args (may be defined only on a subclass).
      // Do not steal nested/local class construction (`Bar()` → bound class call_o).
      if (ctx.current_class && !ctx.in_static_method && ctx.package_scope_cpp.empty() &&
          ctx.current_class->decl && !ctx.current_class->param_names.count(callee) &&
          ctx.current_class->decl->name.compare(0, 7, "Closure") != 0 &&
          ctx.classes.count(class_cpp(*ctx.current_class)) &&
          !ctx.current_class->nested_instance.count(callee) &&
          !ctx.current_class->nested_static.count(callee) && !ctx.classes.count(callee) &&
          !ctx.local_classes.count(callee)) {
        emit_send(ctx, "self", mangle_method(callee, arg_tmps.size()), arg_tmps, dst);
        return;
      }
    }
    // Callable apply: recv(args) → call_o / add_o
    const std::string recv = ctx.fresh("t");
    ctx.out << "  id " << recv << ";\n";
    emit_expr(ctx, *e.lhs, recv);
    ctx.out << "  if (id_is_int(" << recv << ") || id_is_double(" << recv << ")) {\n";
    ctx.out << "    zefc_error(\"attempt to call number\");\n";
    ctx.out << "  }\n";
    emit_send(ctx, recv, "call_o", arg_tmps, dst);
    return;
  }
  }
  throw std::runtime_error("unhandled expression kind");
}

void
emit_expr_top(Ctx& ctx, const Expr& e, const std::string& dst)
{
  emit_line(ctx, e.line);
  emit_expr(ctx, e, dst);
}

void
emit_nested_class(Ctx& ctx, const ClassDecl& c, const std::vector<std::string>& captures)
{
  const std::string& name = c.name;
  const ClassDecl* parent_decl = nullptr;
  std::vector<std::string> parent_caps;
  if (!c.parent.empty()) {
    auto dit = ctx.local_class_decls.find(c.parent);
    if (dit != ctx.local_class_decls.end()) {
      parent_decl = dit->second;
    }
    auto cit = ctx.local_classes.find(c.parent);
    if (cit != ctx.local_classes.end()) {
      parent_caps = cit->second;
    }
  }
  auto is_parent_cap = [&](const std::string& cap) {
    for (const std::string& p : parent_caps) {
      if (p == cap) {
        return true;
      }
    }
    return false;
  };

  // Instance + class-object layouts. Parent fields/captures first so Parent__init
  // can treat a subclass instance as a layout-compatible Parent_*.
  ctx.prelude << "struct " << name << "_ {\n  IsaPtr isa_;\n";
  if (parent_decl) {
    // Walk parent chain for fields + each ancestor's super_inited pad.
    std::vector<const ClassDecl*> chain;
    const ClassDecl* cur = parent_decl;
    while (cur) {
      chain.push_back(cur);
      if (cur->parent.empty()) {
        break;
      }
      auto dit = ctx.local_class_decls.find(cur->parent);
      if (dit != ctx.local_class_decls.end()) {
        cur = dit->second;
        continue;
      }
      auto cit = ctx.classes.find(cur->parent);
      cur = cit == ctx.classes.end() ? nullptr : cit->second.decl;
    }
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
      for (const Field& f : (*it)->fields) {
        if (!f.is_static) {
          ctx.prelude << "  " << kIdType << " " << f.name << ";\n";
        }
      }
      if (!(*it)->parent.empty()) {
        ctx.prelude << "  bool zefc_super_inited_" << (*it)->name << ";\n";
      }
    }
    for (const std::string& cap : parent_caps) {
      ctx.prelude << "  " << kIdType << " " << cap << ";\n";
    }
  }
  for (const Field& f : c.fields) {
    if (!f.is_static) {
      ctx.prelude << "  " << kIdType << " " << f.name << ";\n";
    }
  }
  for (const std::string& cap : captures) {
    if (is_parent_cap(cap)) {
      continue;
    }
    bool is_decl_field = false;
    for (const Field& f : c.fields) {
      if (!f.is_static && f.name == cap) {
        is_decl_field = true;
        break;
      }
    }
    if (!is_decl_field) {
      ctx.prelude << "  " << kIdType << " " << cap << ";\n";
    }
  }
  if (parent_decl) {
    ctx.prelude << "  bool zefc_super_inited;\n";
  }
  ctx.prelude << "};\n\n";
  ctx.prelude << "struct " << name << "Class_ {\n  IsaPtr isa_;\n";
  for (const std::string& cap : captures) {
    ctx.prelude << "  " << kIdType << " " << cap << ";\n";
  }
  ctx.prelude << "};\n";
  ctx.prelude << "static VTable* " << name << "_vtable = nullptr;\n";
  ctx.prelude << "static VTable* " << name << "Class_vtable = nullptr;\n";
  ctx.prelude << "static void ensure_" << name << "();\n";
  ctx.prelude << "static void ensure_" << name << "Class();\n\n";

  ClassInfo info;
  info.decl = &c;
  if (parent_decl) {
    for (const Field& f : parent_decl->fields) {
      if (!f.is_static) {
        info.field_names.insert(f.name);
      }
    }
    for (const std::string& cap : parent_caps) {
      info.field_names.insert(cap);
    }
  }
  for (const Field& f : c.fields) {
    if (!f.is_static) {
      info.field_names.insert(f.name);
    }
  }
  for (const std::string& cap : captures) {
    info.field_names.insert(cap);
  }
  for (const Method& m : c.methods) {
    if (!m.name.empty() && !m.is_static && m.params.empty()) {
      info.method_names.insert(m.name);
    }
  }

  // Accessors + named methods into a temp buffer, then prelude.
  std::ostringstream meth_out;
  {
    Ctx mctx;
    mctx.classes = ctx.classes;
    mctx.functions = ctx.functions;
    mctx.packages = ctx.packages;
    mctx.imports = ctx.imports;
    mctx.local_classes = ctx.local_classes;
    mctx.local_class_decls = ctx.local_class_decls;
    mctx.local_class_static_init = ctx.local_class_static_init;
    mctx.source_path = ctx.source_path;
    mctx.tmp = ctx.tmp;
    mctx.closure_id = ctx.closure_id;
    mctx.current_class = &info;
    mctx.env_params = ctx.env_params;
    // Fields are capturable free names for nested lambdas / classes (test32).
    for (const std::string& fname : info.field_names) {
      mctx.env_params.push_back(fname);
    }

    for (const Field& f : c.fields) {
      if (f.is_static) {
        continue;
      }
      if (f.readable || f.accessible) {
        const std::string g = mangle_getter(f.name);
        mctx.out << "static id\n" << name << "__" << g << "(id self, int selector)\n{\n";
        mctx.out << "  (void)selector;\n";
        if (parent_decl) {
          emit_check_super_inited(mctx, name);
        }
        mctx.out << "  return body<" << name << "_>(self)->" << f.name << ";\n}\n\n";
      }
      if (f.accessible) {
        const std::string s = mangle_setter(f.name);
        mctx.out << "static id\n" << name << "__" << s << "(id self, int selector, id v)\n{\n";
        mctx.out << "  (void)selector;\n";
        if (parent_decl) {
          emit_check_super_inited(mctx, name);
        }
        mctx.out << "  body<" << name << "_>(self)->" << f.name << " = v;\n";
        mctx.out << "  return null_id();\n}\n\n";
      }
    }

    for (const Method& m : c.methods) {
      if (m.name.empty() && m.is_static) {
        mctx.out << "static id\n" << name << "__static_init(id self, int selector)\n{\n";
        mctx.out << "  (void)selector;\n  (void)self;\n";
        mctx.in_static_method = true;
        const std::string tmp = mctx.fresh("t");
        mctx.out << "  id " << tmp << ";\n";
        mctx.allow_return = true;
        mctx.import_binds = true;
        emit_body(mctx, m.body, tmp, false);
        mctx.allow_return = false;
        mctx.in_static_method = false;
        mctx.out << "}\n\n";
        continue;
      }
      if (m.name.empty() || m.is_static) {
        continue;
      }
      info.param_names.clear();
      for (const std::string& p : m.params) {
        info.param_names.insert(p);
      }
      const std::string mangled = mangle_method(m.name, m.params.size());
      mctx.out << "static id\n" << name << "__" << mangled << "(id self, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        mctx.out << ", id " << local_sym(m.params[i]);
      }
      mctx.out << ")\n{\n  (void)selector;\n";
      if (parent_decl) {
        emit_check_super_inited(mctx, name);
      }
      // Snapshot env so nested lambdas/classes can capture params + fields.
      std::vector<std::string> saved_env = mctx.env_params;
      mctx.local_vars.clear();
      for (const std::string& p : m.params) {
        mctx.env_params.push_back(p);
        mctx.local_vars.insert(p);
      }
      const std::string tmp = mctx.fresh("t");
      mctx.out << "  id " << tmp << ";\n";
      mctx.allow_return = true;
      mctx.import_binds = true;
      emit_body(mctx, m.body, tmp, false);
      mctx.allow_return = false;
      mctx.env_params = saved_env;
      mctx.out << "}\n\n";
    }

    // Parent accessors / methods not overridden (function-local subclass; full chain).
    if (parent_decl) {
      emit_inherited_instance_forwards(
          mctx, c, name, c.parent, [&](const std::string& pname) -> const ClassDecl* {
            auto dit = ctx.local_class_decls.find(pname);
            if (dit != ctx.local_class_decls.end()) {
              return dit->second;
            }
            auto cit = ctx.classes.find(pname);
            if (cit != ctx.classes.end()) {
              return cit->second.decl;
            }
            return nullptr;
          });
    }

    const Method* ctor = nullptr;
    for (const Method& m : c.methods) {
      if (m.name.empty() && !m.is_static) {
        ctor = &m;
        break;
      }
    }

    // Name__init: field inits + ctor body (also used by subclass super(...)).
    mctx.out << "static id\n" << name << "__init(id self, int selector";
    if (ctor) {
      for (size_t i = 0; i < ctor->params.size(); ++i) {
        mctx.out << ", id " << local_sym(ctor->params[i]);
      }
    }
    mctx.out << ")\n{\n  (void)selector;\n";
    info.param_names.clear();
    mctx.local_vars.clear();
    std::vector<std::string> saved_env = mctx.env_params;
    if (ctor) {
      for (const std::string& p : ctor->params) {
        info.param_names.insert(p);
        mctx.local_vars.insert(p);
        mctx.env_params.push_back(p);
      }
    }
    for (const Field& f : c.fields) {
      if (f.is_static || !f.init) {
        continue;
      }
      const std::string t = mctx.fresh("t");
      mctx.out << "  id " << t << ";\n";
      emit_expr_top(mctx, *f.init, t);
      mctx.out << "  body<" << name << "_>(self)->" << f.name << " = " << t << ";\n";
    }
    if (ctor) {
      const std::string tmp = mctx.fresh("t");
      mctx.out << "  id " << tmp << ";\n";
      mctx.allow_return = true;
      mctx.import_binds = true;
      emit_body(mctx, ctor->body, tmp, true);
      mctx.allow_return = false;
    } else {
      mctx.out << "  return self;\n";
    }
    mctx.env_params = saved_env;
    mctx.out << "}\n\n";

    // Class object call = allocate + copy captures + __init.
    mctx.out << "static id\n" << name << "Class__call_o(id self, int selector";
    if (ctor) {
      for (size_t i = 0; i < ctor->params.size(); ++i) {
        mctx.out << ", id " << local_sym(ctor->params[i]);
      }
    }
    mctx.out << ")\n{\n  (void)selector;\n";
    mctx.out << "  ensure_" << name << "();\n";
    mctx.out << "  id nest_cls = self;\n";
    mctx.out << "  " << name << "_* self_b = alloc<" << name << "_>();\n";
    mctx.out << "  self = as_id(self_b);\n";
    mctx.out << "  zefc_set_isa(self_b, " << name << "_vtable);\n";
    for (const std::string& cap : captures) {
      mctx.out << "  body<" << name << "_>(self)->" << cap << " = body<" << name
               << "Class_>(nest_cls)->" << cap << ";\n";
    }
    mctx.out << "  return " << name << "__init(self, selector";
    if (ctor) {
      for (const std::string& p : ctor->params) {
        mctx.out << ", " << local_sym(p);
      }
    }
    mctx.out << ");\n";
    mctx.out << "}\n\n";

    // ensure_Name / ensure_NameClass
    mctx.out << "static void\nensure_" << name << "()\n{\n";
    mctx.out << "  if (" << name << "_vtable) {\n    return;\n  }\n";
    mctx.out << "  " << name << "_vtable = vtable_create();\n";
    for (const Field& f : c.fields) {
      if (f.is_static) {
        continue;
      }
      if (f.readable || f.accessible) {
        mctx.out << "  field_register_get(" << name << "_vtable, selector_intern(\""
                 << mangle_getter(f.name) << "\"), offsetof(" << name << "_, " << f.name
                 << "));\n";
        mctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
                 << mangle_getter(f.name) << "\"), " << name << "__" << mangle_getter(f.name)
                 << ");\n";
      }
      if (f.accessible) {
        mctx.out << "  field_register_set(" << name << "_vtable, selector_intern(\""
                 << mangle_setter(f.name) << "\"), offsetof(" << name << "_, " << f.name
                 << "));\n";
        mctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
                 << mangle_setter(f.name) << "\"), " << name << "__" << mangle_setter(f.name)
                 << ");\n";
      }
    }
    if (parent_decl) {
      emit_inherited_vtable_sets(
          mctx, c, name, [&](const std::string& pname) -> const ClassDecl* {
            auto dit = ctx.local_class_decls.find(pname);
            if (dit != ctx.local_class_decls.end()) {
              return dit->second;
            }
            auto cit = ctx.classes.find(pname);
            if (cit != ctx.classes.end()) {
              return cit->second.decl;
            }
            return nullptr;
          });
    }
    for (const Method& m : c.methods) {
      if (m.name.empty() || m.is_static) {
        continue;
      }
      const std::string mangled = mangle_method(m.name, m.params.size());
      mctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\"" << mangled
               << "\"), " << name << "__" << mangled << ");\n";
    }
    mctx.out << "}\n\n";

    mctx.out << "static void\nensure_" << name << "Class()\n{\n";
    mctx.out << "  if (" << name << "Class_vtable) {\n    return;\n  }\n";
    mctx.out << "  ensure_" << name << "();\n";
    mctx.out << "  " << name << "Class_vtable = vtable_create();\n";
    mctx.out << "  vtable_set(" << name << "Class_vtable, selector_intern(\"call_o\"), " << name
             << "Class__call_o);\n";
    mctx.out << "}\n\n";

    ctx.tmp = mctx.tmp;
    ctx.closure_id = mctx.closure_id;
    ctx.prelude << mctx.prelude.str();
    meth_out << mctx.out.str();
  }
  ctx.prelude << meth_out.str();
}

void
emit_block_item(Ctx& ctx, const BlockItem& item, const std::string* result_dst)
{
  emit_line(ctx, item.line);
  if (item.kind == BlockItem::Kind::Import) {
    emit_import_bind(ctx, item.import_path);
    if (result_dst) {
      ctx.out << "  " << *result_dst << " = null_id();\n";
    }
    return;
  }
  if (item.kind == BlockItem::Kind::Class) {
    const ClassDecl& c = *item.nested_class;
    std::vector<std::string> captures;
    collect_free_class(c, ctx.env_params, captures);
    // Subclass class-objects need the parent's free-var captures too (layout prefix).
    if (!c.parent.empty() && ctx.local_classes.count(c.parent)) {
      for (const std::string& cap : ctx.local_classes[c.parent]) {
        bool found = false;
        for (const std::string& existing : captures) {
          if (existing == cap) {
            found = true;
            break;
          }
        }
        if (!found) {
          captures.push_back(cap);
        }
      }
    }
    ctx.local_class_decls[c.name] = &c;
    if (!ctx.nested_emitted.count(c.name)) {
      emit_nested_class(ctx, c, captures);
      ctx.nested_emitted.insert(c.name);
    }
    ctx.local_classes[c.name] = captures;
    for (const Method& m : c.methods) {
      if (m.name.empty() && m.is_static) {
        ctx.local_class_static_init.insert(c.name);
        break;
      }
    }
    if (result_dst) {
      ctx.out << "  {\n";
      ctx.out << "    ensure_" << c.name << "Class();\n";
      ctx.out << "    " << c.name << "Class_* _cls = alloc<" << c.name << "Class_>();\n";
      ctx.out << "    zefc_set_isa(_cls, " << c.name << "Class_vtable);\n";
      for (const std::string& cap : captures) {
        if (is_field(ctx, cap)) {
          ctx.out << "    _cls->" << cap << " = body<" << class_cpp(*ctx.current_class)
                  << "_>(self)->" << cap << ";\n";
        } else {
          ctx.out << "    _cls->" << cap << " = " << local_sym(cap) << ";\n";
        }
      }
      ctx.out << "    " << *result_dst << " = as_id(_cls);\n";
      ctx.out << "  }\n";
    }
    return;
  }
  if (item.kind == BlockItem::Kind::VarDecl) {
    ctx.out << "  id " << local_sym(item.var_name);
    if (item.expr) {
      ctx.out << ";\n";
      emit_expr_top(ctx, *item.expr, local_sym(item.var_name));
      if (item.expr->kind == Expr::Kind::Lambda && item.expr->params.empty()) {
        ctx.local_fn0.insert(item.var_name);
      } else {
        ctx.local_fn0.erase(item.var_name);
      }
    } else {
      ctx.out << " = null_id();\n";
      ctx.local_fn0.erase(item.var_name);
    }
    ctx.env_params.push_back(item.var_name);
    ctx.local_vars.insert(item.var_name);
    if (result_dst) {
      ctx.out << "  " << *result_dst << " = " << local_sym(item.var_name) << ";\n";
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
collect_class_decl(Ctx& ctx, const ClassDecl& c, const std::string& cpp_name, bool top_level)
{
  ClassInfo info;
  info.decl = &c;
  info.cpp_name = cpp_name;
  if (c.parent_expr) {
    info.dynamic_parent = true;
  }
  for (const Field& f : c.fields) {
    if (!f.is_static) {
      info.field_names.insert(f.name);
    }
    if (f.is_private && (f.readable || f.accessible)) {
      info.private_fields.insert(f.name);
    }
    if (f.is_static) {
      info.has_static = true;
    }
  }
  bool has_static_call_method = false;
  for (const Method& m : c.methods) {
    if (m.name.empty() && !m.is_static) {
      info.has_instance_ctor = true;
      if (m.is_private) {
        info.private_ctor = true;
      }
    }
    if (m.is_private && !m.name.empty()) {
      info.private_fields.insert(m.name);
    }
    if (m.is_static) {
      info.has_static = true;
      if (m.name == "call" && m.params.empty()) {
        info.has_static_call0 = true;
        has_static_call_method = true;
      }
      if (!m.name.empty() && m.params.empty()) {
        info.static_methods0.insert(m.name);
      }
    } else if (!m.name.empty()) {
      info.methods[m.name] = m.params.size();
      if (m.params.empty()) {
        info.method_names.insert(m.name);
      }
    }
  }
  for (const ClassDecl::Nested& n : c.nested) {
    const std::string ncpp = cpp_name + "_" + n.decl->name;
    if (n.is_static) {
      info.has_static = true;
      info.nested_static[n.decl->name] = ncpp;
      if (n.decl->name == "call") {
        info.has_static_call0 = true;
        if (!has_static_call_method) {
          info.synth_nested_call = true;
        }
      }
    } else {
      info.nested_instance[n.decl->name] = ncpp;
      info.field_names.insert(n.decl->name);
      if (n.is_private) {
        info.private_fields.insert(n.decl->name);
      }
    }
    collect_class_decl(ctx, *n.decl, ncpp, false);
    ClassInfo& ni = ctx.classes[ncpp];
    ni.enclosing_cpp = cpp_name;
    ni.nest_is_static = n.is_static;
    ni.needs_outer = !n.is_static;
    ni.is_type_nested = true;
    ni.has_static = true; // Class object for Foo.Bar / o.Bar()
  }
  ctx.classes[cpp_name] = info;
  if (top_level) {
    ctx.classes[c.name] = ctx.classes[cpp_name];
  }
}

void
resolve_nested_parents(ClassDecl& c, const std::string& cpp_name)
{
  std::unordered_map<std::string, std::string> nest;
  for (ClassDecl::Nested& n : c.nested) {
    nest[n.decl->name] = cpp_name + "_" + n.decl->name;
  }
  for (ClassDecl::Nested& n : c.nested) {
    if (!n.decl->parent.empty()) {
      auto it = nest.find(n.decl->parent);
      if (it != nest.end()) {
        n.decl->parent = it->second;
      }
    }
    resolve_nested_parents(*n.decl, nest[n.decl->name]);
  }
}

void
collect_package_decl(Ctx& ctx, const PackageDecl& p, const std::string& path)
{
  const std::string cpp = "Pkg_" + path;
  PackageInfo& info = ctx.packages[cpp];
  info.path = path;
  info.cpp_name = cpp;
  for (const Field& f : p.fields) {
    // Keep pointers into Program-owned PackageDecl fields.
    bool found = false;
    for (const Field* existing : info.fields) {
      if (existing->name == f.name) {
        found = true;
        break;
      }
    }
    if (!found) {
      info.fields.push_back(&f);
    }
  }
  if (p.has_ctor) {
    info.has_ctor = true;
    info.ctor_body = &p.ctor_body;
  }
  for (const auto& c : p.classes) {
    const std::string ccpp = path + "_" + c->name;
    if (!info.classes.count(c->name)) {
      resolve_nested_parents(const_cast<ClassDecl&>(*c), ccpp);
      collect_class_decl(ctx, *c, ccpp, false);
      // Package members need a class object even without static methods (foo.Thingy).
      ctx.classes[ccpp].has_static = true;
      info.classes[c->name] = ccpp;
    }
  }
  for (const FuncDecl& f : p.funcs) {
    info.funcs[f.name] = &f;
  }
  for (const auto& nested : p.packages) {
    const std::string npath = path + "_" + nested->name;
    const std::string ncpp = "Pkg_" + npath;
    info.packages[nested->name] = ncpp;
    collect_package_decl(ctx, *nested, npath);
  }
}

static void
import_package_members(Ctx& ctx, const PackageInfo& pkg)
{
  for (const auto& kv : pkg.classes) {
    ImportBinding b;
    b.kind = ImportKind::Class;
    b.cpp = kv.second;
    ctx.imports[kv.first] = b;
  }
  for (const auto& kv : pkg.funcs) {
    ImportBinding b;
    b.kind = ImportKind::Func;
    b.cpp = pkg.cpp_name;
    b.member = kv.first;
    b.arity = kv.second->params.size();
    ctx.imports[kv.first] = b;
  }
  for (const auto& kv : pkg.packages) {
    ImportBinding b;
    b.kind = ImportKind::Package;
    b.cpp = kv.second;
    ctx.imports[kv.first] = b;
  }
  for (const Field* f : pkg.fields) {
    // Import readable/accessible fields as zero-arg getters on the package.
    if (f->readable || f->accessible) {
      ImportBinding b;
      b.kind = ImportKind::Func;
      b.cpp = pkg.cpp_name;
      b.member = f->name;
      b.arity = 0;
      ctx.imports[f->name] = b;
    }
  }
}

void
collect_classes(Ctx& ctx, const Program& program)
{
  for (const Stmt& s : program.stmts) {
    if (s.kind == Stmt::Kind::Func) {
      if (ctx.functions.count(s.func_decl.name)) {
        throw std::runtime_error("parse error at line " + std::to_string(s.line) +
                                 ": cannot redeclare function named " + s.func_decl.name);
      }
      FuncInfo fi;
      fi.decl = &s.func_decl;
      fi.arity = s.func_decl.params.size();
      ctx.functions[s.func_decl.name] = fi;
      continue;
    }
    if (s.kind == Stmt::Kind::Package) {
      collect_package_decl(ctx, s.package_decl, s.package_decl.name);
      ctx.package_cpp[s.package_decl.name] = "Pkg_" + s.package_decl.name;
      continue;
    }
    if (s.kind == Stmt::Kind::Import) {
      if (s.import_path.empty()) {
        throw std::runtime_error("empty import");
      }
      const PackageInfo* pkg = nullptr;
      if (s.import_path.size() == 1) {
        const std::string& name = s.import_path[0];
        auto pit = ctx.package_cpp.find(name);
        if (pit != ctx.package_cpp.end()) {
          pkg = &ctx.packages.at(pit->second);
        } else if (ctx.imports.count(name) && ctx.imports[name].kind == ImportKind::Package) {
          pkg = &ctx.packages.at(ctx.imports[name].cpp);
        } else {
          throw std::runtime_error("import of unknown package " + name);
        }
      } else {
        auto pit = ctx.package_cpp.find(s.import_path[0]);
        if (pit == ctx.package_cpp.end()) {
          throw std::runtime_error("import of unknown package " + s.import_path[0]);
        }
        pkg = &ctx.packages.at(pit->second);
        for (size_t i = 1; i < s.import_path.size(); ++i) {
          auto nit = pkg->packages.find(s.import_path[i]);
          if (nit == pkg->packages.end()) {
            throw std::runtime_error("import path missing " + s.import_path[i]);
          }
          pkg = &ctx.packages.at(nit->second);
        }
      }
      import_package_members(ctx, *pkg);
      continue;
    }
    if (s.kind != Stmt::Kind::Class) {
      continue;
    }
    resolve_nested_parents(const_cast<ClassDecl&>(s.class_decl), s.class_decl.name);
    collect_class_decl(ctx, s.class_decl, s.class_decl.name, true);
  }
  // Second pass: merge parent field names (reject cycles first).
  for (auto& kv : ctx.classes) {
    std::unordered_set<std::string> seen;
    std::string p = kv.second.decl->parent;
    while (!p.empty()) {
      if (p == kv.first || !seen.insert(p).second) {
        throw std::runtime_error("cannot create cyclic class hierarchy");
      }
      auto it = ctx.classes.find(p);
      if (it == ctx.classes.end()) {
        break;
      }
      if (it->second.decl->is_final) {
        throw std::runtime_error("cannot subclass final class");
      }
      for (const Field& f : it->second.decl->fields) {
        if (!f.is_static) {
          kv.second.field_names.insert(f.name);
        }
      }
      for (const Method& m : it->second.decl->methods) {
        if (m.name.empty() || m.is_static) {
          continue;
        }
        if (!kv.second.methods.count(m.name)) {
          kv.second.methods[m.name] = m.params.size();
        }
        if (m.params.empty()) {
          kv.second.method_names.insert(m.name);
        }
      }
      for (const auto& nk : it->second.nested_instance) {
        kv.second.nested_instance.insert(nk);
        kv.second.field_names.insert(nk.first);
      }
      p = it->second.decl->parent;
    }
  }
  // Instance-nested outer pointer: only on the nest root layout (not subclasses).
  for (auto& kv : ctx.classes) {
    if (!kv.second.needs_outer) {
      continue;
    }
    bool parent_outer = false;
    std::string p = kv.second.decl->parent;
    while (!p.empty()) {
      auto it = ctx.classes.find(p);
      if (it == ctx.classes.end()) {
        break;
      }
      if (it->second.needs_outer) {
        parent_outer = true;
        break;
      }
      p = it->second.decl->parent;
    }
    kv.second.has_outer_field = !parent_outer;
  }
}

void
emit_package(Ctx& ctx, const PackageInfo& pkg)
{
  const std::string& name = pkg.cpp_name;
  for (const auto& kv : pkg.packages) {
    emit_package(ctx, ctx.packages.at(kv.second));
  }
  ctx.package_scope_cpp = name;
  ctx.package_scope_fields.clear();
  for (const Field* f : pkg.fields) {
    ctx.package_scope_fields.insert(f->name);
  }
  ctx.package_locals = pkg.classes;
  for (const auto& kv : pkg.classes) {
    const ClassInfo& ci = ctx.classes.at(kv.second);
    emit_class(ctx, *ci.decl, kv.second);
  }

  ctx.out << "struct " << name << "_ {\n  IsaPtr isa_;\n";
  for (const Field* f : pkg.fields) {
    ctx.out << "  " << kIdType << " " << f->name << ";\n";
  }
  for (const auto& kv : pkg.classes) {
    ctx.out << "  " << kIdType << " " << kv.first << ";\n";
  }
  for (const auto& kv : pkg.packages) {
    ctx.out << "  " << kIdType << " " << kv.first << ";\n";
  }
  ctx.out << "};\n";
  ctx.out << "static " << name << "_ g_" << name << ";\n";
  ctx.out << "static VTable* " << name << "_vtable = nullptr;\n";
  ctx.out << "static bool g_" << name << "_inited = false;\n";
  ctx.out << "static bool g_" << name << "_constructed = false;\n\n";

  ctx.out << "static void ensure_" << name << "();\n";
  ctx.out << "static void ensure_" << name << "_constructed();\n";
  if (pkg.has_ctor) {
    ctx.out << "static id " << name << "__init(id self, int selector);\n";
  }
  for (const Field* f : pkg.fields) {
    if (f->readable || f->accessible) {
      ctx.out << "static id " << name << "__" << mangle_getter(f->name)
              << "(id self, int selector);\n";
    }
    if (f->accessible) {
      ctx.out << "static id " << name << "__" << mangle_setter(f->name)
              << "(id self, int selector, id v);\n";
    }
  }
  for (const auto& kv : pkg.classes) {
    ctx.out << "static id " << name << "__" << mangle_getter(kv.first)
            << "(id self, int selector);\n";
  }
  for (const auto& kv : pkg.packages) {
    ctx.out << "static id " << name << "__" << mangle_getter(kv.first)
            << "(id self, int selector);\n";
  }
  for (const auto& kv : pkg.funcs) {
    const std::string mangled = mangle_method(kv.first, kv.second->params.size());
    ctx.out << "static id " << name << "__" << mangled << "(id self, int selector";
    for (size_t i = 0; i < kv.second->params.size(); ++i) {
      ctx.out << ", id p" << i;
    }
    ctx.out << ");\n";
  }
  ctx.out << "\n";

  std::ostringstream saved_out;
  saved_out << ctx.out.str();
  ctx.out.str("");
  ctx.out.clear();
  ctx.last_line = -1;

  // Fake class env for package methods / ctor / field access via self.
  ClassDecl fake_decl;
  fake_decl.name = name;
  for (const Field* f : pkg.fields) {
    Field ff;
    ff.name = f->name;
    fake_decl.fields.push_back(std::move(ff));
  }
  ClassInfo fake_info;
  fake_info.decl = &fake_decl;
  fake_info.cpp_name = name;
  for (const Field* f : pkg.fields) {
    fake_info.field_names.insert(f->name);
  }
  for (const auto& kv : pkg.funcs) {
    if (kv.second->params.empty()) {
      fake_info.method_names.insert(kv.first);
    }
    ctx.package_methods[kv.first] = kv.second->params.size();
  }
  ctx.package_locals = pkg.classes;

  auto emit_pkg_method_prologue = [&]() {
    ctx.out << "  ensure_" << name << "_constructed();\n";
  };

  for (const Field* f : pkg.fields) {
    if (f->readable || f->accessible) {
      ctx.out << "static id\n" << name << "__" << mangle_getter(f->name)
              << "(id self, int selector)\n{\n  (void)selector;\n";
      emit_pkg_method_prologue();
      ctx.out << "  return body<" << name << "_>(self)->" << f->name << ";\n}\n\n";
    }
    if (f->accessible) {
      ctx.out << "static id\n" << name << "__" << mangle_setter(f->name)
              << "(id self, int selector, id v)\n{\n  (void)selector;\n";
      emit_pkg_method_prologue();
      ctx.out << "  body<" << name << "_>(self)->" << f->name << " = v;\n";
      ctx.out << "  return null_id();\n}\n\n";
    }
  }
  for (const auto& kv : pkg.classes) {
    ctx.out << "static id\n" << name << "__" << mangle_getter(kv.first)
            << "(id self, int selector)\n{\n  (void)selector;\n";
    emit_pkg_method_prologue();
    ctx.out << "  return body<" << name << "_>(self)->" << kv.first << ";\n}\n\n";
  }
  for (const auto& kv : pkg.packages) {
    ctx.out << "static id\n" << name << "__" << mangle_getter(kv.first)
            << "(id self, int selector)\n{\n  (void)selector;\n";
    emit_pkg_method_prologue();
    ctx.out << "  return body<" << name << "_>(self)->" << kv.first << ";\n}\n\n";
  }

  if (pkg.has_ctor && pkg.ctor_body) {
    ctx.out << "static id\n" << name << "__init(id self, int selector)\n{\n  (void)selector;\n";
    ctx.current_class = &fake_info;
    ctx.env_params = ctx.toplevel_vars;
    for (const Field* f : pkg.fields) {
      ctx.env_params.push_back(f->name);
    }
    const std::string tmp = ctx.fresh("t");
    ctx.out << "  id " << tmp << ";\n";
    ctx.local_vars.clear();
    ctx.allow_return = true;
    ctx.import_binds = true;
    emit_body(ctx, *pkg.ctor_body, tmp, true);
    ctx.allow_return = false;
    ctx.import_binds = false;
    ctx.env_params.clear();
    ctx.local_vars.clear();
    ctx.current_class = nullptr;
    ctx.out << "}\n\n";
  }

  for (const auto& kv : pkg.funcs) {
    const FuncDecl& f = *kv.second;
    const std::string mangled = mangle_method(f.name, f.params.size());
    ctx.out << "static id\n" << name << "__" << mangled << "(id self, int selector";
    for (size_t i = 0; i < f.params.size(); ++i) {
      ctx.out << ", id " << local_sym(f.params[i]);
    }
    ctx.out << ")\n{\n  (void)selector;\n";
    emit_pkg_method_prologue();
    ctx.current_class = &fake_info;
    fake_info.param_names.clear();
    for (const std::string& p : f.params) {
      fake_info.param_names.insert(p);
    }
    ctx.env_params = ctx.toplevel_vars;
    ctx.local_vars.clear();
    for (const Field* fild : pkg.fields) {
      ctx.env_params.push_back(fild->name);
    }
    for (const std::string& p : f.params) {
      ctx.env_params.push_back(p);
      ctx.local_vars.insert(p);
    }
    if (f.params.empty()) {
      // Load-rebindable: seed slot from body once, then always read the slot.
      ctx.out << "  if (!package_slot_has(\"" << pkg.path << "\", \"" << f.name << "\")) {\n";
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      if (f.body.empty()) {
        ctx.out << "  " << tmp << " = null_id();\n";
      } else {
        for (size_t i = 0; i + 1 < f.body.size(); ++i) {
          emit_block_item(ctx, f.body[i], nullptr);
        }
        emit_block_item(ctx, f.body.back(), &tmp);
      }
      ctx.out << "  package_slot_set(\"" << pkg.path << "\", \"" << f.name << "\", " << tmp
              << ");\n";
      ctx.out << "  }\n";
      ctx.out << "  return package_slot_get(\"" << pkg.path << "\", \"" << f.name << "\");\n";
    } else {
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      ctx.allow_return = true;
      ctx.import_binds = true;
      emit_body(ctx, f.body, tmp, false);
      ctx.allow_return = false;
      ctx.import_binds = false;
    }
    ctx.env_params.clear();
    ctx.local_vars.clear();
    ctx.current_class = nullptr;
    ctx.out << "}\n\n";
  }

  ctx.out << "static void\nensure_" << name << "()\n{\n";
  ctx.out << "  if (g_" << name << "_inited) {\n    return;\n  }\n";
  for (const auto& kv : pkg.packages) {
    ctx.out << "  ensure_" << kv.second << "();\n";
  }
  for (const auto& kv : pkg.classes) {
    ctx.out << "  ensure_" << kv.second << "();\n";
  }
  ctx.out << "  " << name << "_vtable = vtable_create();\n";
  ctx.out << "  zefc_set_isa(&g_" << name << ", " << name << "_vtable);\n";
  for (const Field* f : pkg.fields) {
    if (f->readable || f->accessible) {
      ctx.out << "  field_register_get(" << name << "_vtable, selector_intern(\""
              << mangle_getter(f->name) << "\"), offsetof(" << name << "_, " << f->name
              << "));\n";
      ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
              << mangle_getter(f->name) << "\"), " << name << "__" << mangle_getter(f->name)
              << ");\n";
    }
    if (f->accessible) {
      ctx.out << "  field_register_set(" << name << "_vtable, selector_intern(\""
              << mangle_setter(f->name) << "\"), offsetof(" << name << "_, " << f->name
              << "));\n";
      ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
              << mangle_setter(f->name) << "\"), " << name << "__" << mangle_setter(f->name)
              << ");\n";
    }
  }
  for (const auto& kv : pkg.classes) {
    ctx.out << "  field_register_get(" << name << "_vtable, selector_intern(\""
            << mangle_getter(kv.first) << "\"), offsetof(" << name << "_, " << kv.first
            << "));\n";
    ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
            << mangle_getter(kv.first) << "\"), " << name << "__" << mangle_getter(kv.first)
            << ");\n";
    ctx.out << "  g_" << name << "." << kv.first << " = as_id(&g_" << kv.second
            << "Class);\n";
  }
  for (const auto& kv : pkg.packages) {
    ctx.out << "  field_register_get(" << name << "_vtable, selector_intern(\""
            << mangle_getter(kv.first) << "\"), offsetof(" << name << "_, " << kv.first
            << "));\n";
    ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
            << mangle_getter(kv.first) << "\"), " << name << "__" << mangle_getter(kv.first)
            << ");\n";
    ctx.out << "  g_" << name << "." << kv.first << " = as_id(&g_" << kv.second << ");\n";
  }
  for (const auto& kv : pkg.funcs) {
    const std::string mangled = mangle_method(kv.first, kv.second->params.size());
    ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\"" << mangled
            << "\"), " << name << "__" << mangled << ");\n";
  }
  // Field initializers (package class names resolve via package_locals).
  ctx.package_locals = pkg.classes;
  ctx.current_class = nullptr;
  for (const Field* f : pkg.fields) {
    if (!f->init) {
      ctx.out << "  g_" << name << "." << f->name << " = null_id();\n";
      continue;
    }
    const std::string t = ctx.fresh("t");
    ctx.out << "  id " << t << ";\n";
    emit_expr_top(ctx, *f->init, t);
    ctx.out << "  g_" << name << "." << f->name << " = " << t << ";\n";
  }
  ctx.package_locals.clear();
  ctx.out << "  g_" << name << "_inited = true;\n";
  ctx.out << "}\n\n";

  ctx.out << "static void\nensure_" << name << "_constructed()\n{\n";
  ctx.out << "  ensure_" << name << "();\n";
  ctx.out << "  if (g_" << name << "_constructed) {\n    return;\n  }\n";
  ctx.out << "  g_" << name << "_constructed = true;\n";
  if (pkg.has_ctor) {
    ctx.out << "  (void)" << name << "__init(as_id(&g_" << name << "), 0);\n";
  }
  ctx.out << "}\n\n";

  ctx.package_locals.clear();
  ctx.package_methods.clear();
  ctx.package_scope_cpp.clear();
  ctx.package_scope_fields.clear();
  ctx.class_methods << ctx.out.str();
  ctx.out.str("");
  ctx.out.clear();
  ctx.last_line = -1;
  ctx.out << saved_out.str();
}

void
emit_accessor_methods(Ctx& ctx, const ClassDecl& c, const std::string& name)
{
  const bool need_super = !c.parent.empty() || c.parent_expr != nullptr;
  for (const Field& f : c.fields) {
    if (f.is_static) {
      continue;
    }
    if (f.readable || f.accessible) {
      const std::string g = mangle_getter(f.name);
      ctx.out << "static id\n" << name << "__" << g << "(id self, int selector)\n{\n";
      ctx.out << "  (void)selector;\n";
      if (need_super) {
        emit_check_super_inited(ctx, name);
      }
      ctx.out << "  return body<" << name << "_>(self)->" << f.name << ";\n}\n\n";
    }
    if (f.accessible) {
      const std::string s = mangle_setter(f.name);
      ctx.out << "static id\n" << name << "__" << s << "(id self, int selector, id v)\n{\n";
      ctx.out << "  (void)selector;\n";
      if (need_super) {
        emit_check_super_inited(ctx, name);
      }
      ctx.out << "  body<" << name << "_>(self)->" << f.name << " = v;\n";
      ctx.out << "  return null_id();\n}\n\n";
    }
  }
  for (const ClassDecl::Nested& n : c.nested) {
    if (n.is_static || n.is_private) {
      continue;
    }
    const std::string g = mangle_getter(n.decl->name);
    const std::string ncpp = name + "_" + n.decl->name;
    ctx.out << "static id\n" << name << "__" << g << "(id self, int selector)\n{\n";
    ctx.out << "  (void)selector;\n";
    if (need_super) {
      emit_check_super_inited(ctx, name);
    }
    // Bound class object with live outer (not the global Class singleton).
    ctx.out << "  id _bound;\n";
    emit_bound_nested_class(ctx, ncpp, "self", "_bound");
    ctx.out << "  return _bound;\n}\n\n";
  }
}

void
emit_class(Ctx& ctx, const ClassDecl& c, const std::string& cpp_name)
{
  const std::string name = cpp_name.empty() ? c.name : cpp_name;
  for (const ClassDecl::Nested& n : c.nested) {
    emit_class(ctx, *n.decl, name + "_" + n.decl->name);
  }

  ClassInfo* info = &ctx.classes[name];

  ctx.out << "struct " << name << "_ {\n  IsaPtr isa_;\n";
  {
    // Parent layout must be an exact prefix (including each ancestor's
    // zefc_super_inited) so body<Parent_>(subclass)->zefc_super_inited works.
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
      const ClassInfo& ai = ctx.classes[*it];
      for (const Field& f : ai.decl->fields) {
        if (!f.is_static) {
          ctx.out << "  " << kIdType << " " << f.name << ";\n";
        }
      }
      for (const ClassDecl::Nested& n : ai.decl->nested) {
        if (!n.is_static) {
          ctx.out << "  " << kIdType << " " << n.decl->name << ";\n";
        }
      }
      if (ai.has_outer_field) {
        ctx.out << "  " << kIdType << " zefc_outer;\n";
      }
      if (!ai.decl->parent.empty()) {
        // Unique name in this struct; Parent_ still sees it as zefc_super_inited
        // at this offset via reinterpret_cast.
        ctx.out << "  bool zefc_super_inited_" << *it << ";\n";
      }
    }
  }
  for (const Field& f : c.fields) {
    if (!f.is_static) {
      ctx.out << "  " << kIdType << " " << f.name << ";\n";
    }
  }
  for (const ClassDecl::Nested& n : c.nested) {
    if (!n.is_static) {
      ctx.out << "  " << kIdType << " " << n.decl->name << ";\n";
    }
  }
  if (info->has_outer_field) {
    ctx.out << "  " << kIdType << " zefc_outer;\n";
  }
  if (info->dynamic_parent) {
    // Composition stand-in for dynamic inheritance (parent instance; freeze-safe).
    ctx.out << "  " << kIdType << " zefc_super;\n";
    ctx.out << "  bool zefc_super_inited;\n";
  } else if (!c.parent.empty()) {
    // Trailing flag: this class called super (Zef parent Storage* link).
    ctx.out << "  bool zefc_super_inited;\n";
  }
  ctx.out << "};\n\n";
  ctx.out << "static VTable* " << name << "_vtable = nullptr;\n\n";
  if (info->dynamic_parent) {
    ctx.out << "static id g_" << name << "_dyn_parent = nullptr;\n";
    ctx.out << "static bool g_" << name << "_parent_resolved = false;\n\n";
  }

  if (info->has_static) {
    ctx.out << "struct " << name << "Class_ {\n  IsaPtr isa_;\n";
    if (info->needs_outer) {
      ctx.out << "  " << kIdType << " zefc_outer;\n";
    }
    for (const Field& f : c.fields) {
      if (f.is_static) {
        ctx.out << "  " << kIdType << " " << f.name << ";\n";
      }
    }
    for (const ClassDecl::Nested& n : c.nested) {
      if (n.is_static) {
        ctx.out << "  " << kIdType << " " << n.decl->name << ";\n";
      }
    }
    ctx.out << "};\n";
    ctx.out << "static " << name << "Class_ g_" << name << "Class;\n";
    ctx.out << "static VTable* " << name << "Class_vtable = nullptr;\n";
    ctx.out << "static bool g_" << name << "Class_inited = false;\n\n";
  }

  bool has_ctor = false;
  for (const Method& m : c.methods) {
    if (m.name.empty() && !m.is_static) {
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
    } else if (m.name.empty() && m.is_static) {
      ctx.out << "static id " << name << "__static_init(id self, int selector);\n";
    } else {
      const std::string mangled = mangle_method(m.name, m.params.size());
      ctx.out << "static id " << method_sym(name, m.is_static, mangled)
              << "(id self, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id p" << i;
      }
      ctx.out << ");\n";
    }
  }
  if (info->synth_nested_call) {
    ctx.out << "static id " << name << "__call_o(id self, int selector);\n";
  }
  // Type-nested construction via class() — but `static fn call` owns call_o.
  if (info->is_type_nested && !info->has_static_call0) {
    ctx.out << "static id " << name << "Class__call_o(id self, int selector";
    for (const Method& m : c.methods) {
      if (m.name.empty()) {
        for (size_t i = 0; i < m.params.size(); ++i) {
          ctx.out << ", id p" << i;
        }
        break;
      }
    }
    ctx.out << ");\n";
  }
  ctx.out << "static void ensure_" << name << "();\n";
  if (!has_ctor) {
    // No synthetic empty constructor (Zef nocons). Still declare __new for call sites.
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
  for (const ClassDecl::Nested& n : c.nested) {
    if (!n.is_static && !n.is_private) {
      ctx.out << "static id " << name << "__" << mangle_getter(n.decl->name)
              << "(id self, int selector);\n";
    }
  }
  ctx.out << "\n";

  emit_accessor_methods(ctx, c, name);

  ctx.current_class = info;

  std::ostringstream saved_out;
  saved_out << ctx.out.str();
  ctx.out.str("");
  ctx.out.clear();
  ctx.last_line = -1;

  auto emit_ctor = [&](const Method* m) {
    ctx.out << "static id\n" << name << "__init(id self, int selector";
    if (m) {
      for (size_t i = 0; i < m->params.size(); ++i) {
        ctx.out << ", id " << local_sym(m->params[i]);
      }
    }
    ctx.out << ")\n{\n  (void)selector;\n";
    ctx.env_params.clear();
    ctx.local_vars.clear();
    info->param_names.clear();
    // Ctor params must be visible to field initializers (`baz = inBaz`).
    if (m) {
      for (const std::string& p : m->params) {
        info->param_names.insert(p);
        ctx.env_params.push_back(p);
        ctx.local_vars.insert(p);
      }
    }
    for (const Field& f : c.fields) {
      if (!f.is_static) {
        ctx.env_params.push_back(f.name);
      }
    }
    for (const ClassDecl::Nested& n : c.nested) {
      if (!n.is_static) {
        ctx.env_params.push_back(n.decl->name);
      }
    }
    for (const Field& f : c.fields) {
      if (f.is_static || !f.init) {
        continue;
      }
      const std::string t = ctx.fresh("t");
      ctx.out << "  id " << t << ";\n";
      emit_expr_top(ctx, *f.init, t);
      ctx.out << "  body<" << name << "_>(self)->" << f.name << " = " << t << ";\n";
    }
    for (const ClassDecl::Nested& n : c.nested) {
      if (n.is_static) {
        continue;
      }
      const std::string ncpp = name + "_" + n.decl->name;
      ctx.out << "  ensure_" << ncpp << "();\n";
      ctx.out << "  body<" << name << "_>(self)->" << n.decl->name << " = as_id(&g_" << ncpp
              << "Class);\n";
    }
    if (m) {
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      ctx.allow_return = true;
      ctx.import_binds = true;
      emit_body(ctx, m->body, tmp, true);
      ctx.allow_return = false;
      ctx.import_binds = false;
    } else {
      ctx.out << "  return self;\n";
    }
    ctx.env_params.clear();
    ctx.local_vars.clear();
    ctx.out << "}\n\n";

    ctx.out << "static id\n" << name << "__new(id, int selector";
    if (m) {
      for (size_t i = 0; i < m->params.size(); ++i) {
        ctx.out << ", id " << local_sym(m->params[i]);
      }
    }
    ctx.out << ")\n{\n  (void)selector;\n";
    ctx.out << "  ensure_" << name << "();\n";
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
    if (m.name.empty() && m.is_static) {
      ctx.out << "static id\n" << name << "__static_init(id self, int selector)\n{\n";
      ctx.out << "  (void)selector;\n";
      ctx.in_static_method = true;
      ctx.env_params.clear();
      ctx.local_vars.clear();
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      ctx.allow_return = true;
      ctx.import_binds = true;
      emit_body(ctx, m.body, tmp, false);
      ctx.allow_return = false;
      ctx.import_binds = false;
      ctx.env_params.clear();
      ctx.local_vars.clear();
      ctx.in_static_method = false;
      ctx.out << "}\n\n";
      continue;
    }
    if (m.name.empty()) {
      emit_ctor(&m);
      emitted_ctor = true;
    } else {
      const std::string mangled = mangle_method(m.name, m.params.size());
      ctx.out << "static id\n" << method_sym(name, m.is_static, mangled)
              << "(id self, int selector";
      for (size_t i = 0; i < m.params.size(); ++i) {
        ctx.out << ", id " << local_sym(m.params[i]);
      }
      ctx.out << ")\n{\n  (void)selector;\n";
      ctx.in_static_method = m.is_static;
      if (!m.is_static && (!c.parent.empty() || info->dynamic_parent)) {
        emit_check_super_inited(ctx, name);
      }
      ctx.env_params.clear();
      ctx.local_vars.clear();
      for (const Field& f : c.fields) {
        if (!f.is_static) {
          ctx.env_params.push_back(f.name);
        }
      }
      for (const std::string& p : m.params) {
        ctx.env_params.push_back(p);
        ctx.local_vars.insert(p);
      }
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      ctx.allow_return = true;
      ctx.import_binds = true;
      emit_body(ctx, m.body, tmp, false);
      ctx.allow_return = false;
      ctx.import_binds = false;
      ctx.env_params.clear();
      ctx.local_vars.clear();
      ctx.in_static_method = false;
      ctx.out << "}\n\n";
    }
  }
  if (!emitted_ctor) {
    // Zef: class with no constructor cannot be instantiated.
    ctx.out << "static id\n" << name << "__new(id, int selector)\n{\n";
    ctx.out << "  (void)selector;\n";
    ctx.out << "  zefc_error(\"cannot instantiate class with no constructor\");\n";
    ctx.out << "}\n\n";
  }
  if (info->synth_nested_call) {
    ctx.out << "static id\n" << name << "__call_o(id self, int selector)\n{\n";
    ctx.out << "  (void)selector;\n";
    ctx.out << "  id nested = body<" << name << "Class_>(self)->call;\n";
    ctx.out << "  return ZEFC_SEND0(nested, " << sel_expr("call_o") << ");\n";
    ctx.out << "}\n\n";
  }
  if (info->is_type_nested && !info->has_static_call0) {
    const Method* ctor = nullptr;
    for (const Method& m : c.methods) {
      if (m.name.empty()) {
        ctor = &m;
        break;
      }
    }
    ctx.out << "static id\n" << name << "Class__call_o(id self, int selector";
    if (ctor) {
      for (size_t i = 0; i < ctor->params.size(); ++i) {
        ctx.out << ", id " << local_sym(ctor->params[i]);
      }
    }
    ctx.out << ")\n{\n  (void)selector;\n";
    ctx.out << "  ensure_" << name << "();\n";
    ctx.out << "  " << name << "_* self_b = alloc<" << name << "_>();\n";
    ctx.out << "  id inst = as_id(self_b);\n";
    ctx.out << "  zefc_set_isa(self_b, " << name << "_vtable);\n";
    if (info->needs_outer) {
      ctx.out << "  self_b->zefc_outer = body<" << name << "Class_>(self)->zefc_outer;\n";
    }
    if (ctor || emitted_ctor) {
      ctx.out << "  return " << name << "__init(inst, selector";
      if (ctor) {
        for (const std::string& p : ctor->params) {
          ctx.out << ", " << local_sym(p);
        }
      }
      ctx.out << ");\n";
    } else {
      ctx.out << "  zefc_error(\"cannot instantiate class with no constructor\");\n";
    }
    ctx.out << "}\n\n";
  }

  // Inherited ancestor methods/accessors: wrap with super-inited check (Zef checkConstructed).
  if (!c.parent.empty()) {
    auto pit = ctx.classes.find(c.parent);
    if (pit != ctx.classes.end()) {
      emit_inherited_instance_forwards(
          ctx, c, name, class_cpp(pit->second),
          [&](const std::string& pname) -> const ClassDecl* {
            auto it = ctx.classes.find(pname);
            return it == ctx.classes.end() ? nullptr : it->second.decl;
          });
    }
  }

  ctx.out << "static void\nensure_" << name << "()\n{\n";
  ctx.out << "  if (" << name << "_vtable) {\n    return;\n  }\n";
  if (!c.parent.empty()) {
    ctx.out << "  ensure_" << c.parent << "();\n";
  }
  for (const ClassDecl::Nested& n : c.nested) {
    ctx.out << "  ensure_" << name << "_" << n.decl->name << "();\n";
  }
  ctx.out << "  " << name << "_vtable = vtable_create();\n";
  if (info->dynamic_parent && c.parent_expr) {
    // Resolve parent once before installing methods / static init (Zef ClassObject::resolve).
    ctx.out << "  if (!g_" << name << "_parent_resolved) {\n";
    ctx.out << "    id _par;\n";
    {
      const bool saved = ctx.allow_unresolved;
      ctx.allow_unresolved = true;
      ctx.current_class = nullptr;
      ctx.env_params = ctx.toplevel_vars;
      emit_expr_top(ctx, *c.parent_expr, "_par");
      ctx.allow_unresolved = saved;
      ctx.current_class = info;
    }
    ctx.out << "    g_" << name << "_dyn_parent = _par;\n";
    ctx.out << "    g_" << name << "_parent_resolved = true;\n";
    ctx.out << "  }\n";
  }
  if (!c.parent.empty()) {
    emit_inherited_vtable_sets(ctx, c, name, [&](const std::string& pname) -> const ClassDecl* {
      auto it = ctx.classes.find(pname);
      return it == ctx.classes.end() ? nullptr : it->second.decl;
    });
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
  for (const ClassDecl::Nested& n : c.nested) {
    if (n.is_static || n.is_private) {
      continue;
    }
    // Method getter (allocates bound Class); not a field IC slot.
    ctx.out << "  vtable_set(" << name << "_vtable, selector_intern(\""
            << mangle_getter(n.decl->name) << "\"), " << name << "__"
            << mangle_getter(n.decl->name) << ");\n";
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
              << "\"), " << method_sym(name, true, mangled) << ");\n";
    }
    if (info->synth_nested_call) {
      ctx.out << "    vtable_set(" << name << "Class_vtable, selector_intern(\"call_o\"), "
              << name << "__call_o);\n";
    }
    if (info->is_type_nested && !info->has_static_call0) {
      ctx.out << "    vtable_set(" << name << "Class_vtable, selector_intern(\"call_o\"), "
              << name << "Class__call_o);\n";
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
    for (const ClassDecl::Nested& n : c.nested) {
      if (!n.is_static) {
        continue;
      }
      const std::string ncpp = name + "_" + n.decl->name;
      ctx.out << "    g_" << name << "Class." << n.decl->name << " = as_id(&g_" << ncpp
              << "Class);\n";
    }
    for (const Method& m : c.methods) {
      if (m.is_static && m.name.empty()) {
        ctx.out << "    (void)" << name << "__static_init(as_id(&g_" << name
                << "Class), 0);\n";
      }
    }
    ctx.out << "    g_" << name << "Class_inited = true;\n";
    ctx.out << "  }\n";
  }
  ctx.out << "}\n\n";

  ctx.current_class = nullptr;

  ctx.class_methods << ctx.out.str();
  ctx.out.str("");
  ctx.out.clear();
  ctx.last_line = -1;
  ctx.out << saved_out.str();
}


} // namespace

std::string
codegen_cpp(const Program& program, const std::string& source_path)
{
  Ctx ctx;
  ctx.source_path = source_path;
  collect_classes(ctx, program);

  ctx.out << "// Generated by zefc " << ZEFC_VERSION << " (" << ZEFC_GIT_DESCRIBE << ")\n";
  ctx.out << "// from " << source_path << "\n";
  ctx.out << "// object_dispatch selected at C++ compile time (-DZEFC_OBJECT_DISPATCH).\n\n";
  ctx.out << "#include \"zefc/array_api.hpp\"\n";
  ctx.out << "#include \"zefc/dispatch.hpp\"\n";
  ctx.out << "#include \"zefc/double_api.hpp\"\n";
  ctx.out << "#include \"zefc/error.hpp\"\n";
  ctx.out << "#include \"zefc/field_ic.hpp\"\n";
  ctx.out << "#include \"zefc/int_api.hpp\"\n";
  ctx.out << "#include \"zefc/io.hpp\"\n";
  ctx.out << "#include \"zefc/known_selectors.hpp\"\n";
  ctx.out << "#include \"zefc/module.hpp\"\n";
  ctx.out << "#include \"zefc/runtime.hpp\"\n";
  ctx.out << "#include \"zefc/runtime_bootstrap.hpp\"\n";
  ctx.out << "#include \"zefc/string_api.hpp\"\n\n";
  ctx.out << "#include <cstddef>\n";
  ctx.out << "#include <cstdlib>\n\n";
  ctx.out << "using namespace zefc;\n\n";
  ctx.out << "namespace {\n\n";

  for (const Stmt& s : program.stmts) {
    if (s.kind == Stmt::Kind::VarDecl) {
      ctx.toplevel_vars.push_back(s.var_name);
      ctx.out << "static id " << local_sym(s.var_name) << ";\n";
    }
  }
  if (!ctx.toplevel_vars.empty()) {
    ctx.out << "\n";
  }
  {
    std::unordered_set<std::string> top_vars(ctx.toplevel_vars.begin(), ctx.toplevel_vars.end());
    for (const Stmt& s : program.stmts) {
      if (s.kind != Stmt::Kind::Func) {
        continue;
      }
      if (top_vars.count(s.func_decl.name) ||
          (ctx.functions.count(s.func_decl.name) &&
           ctx.functions[s.func_decl.name].decl != &s.func_decl)) {
        throw std::runtime_error("parse error at line " + std::to_string(s.line) +
                                 ": cannot redeclare function named " + s.func_decl.name);
      }
    }
  }

  for (const Stmt& s : program.stmts) {
    if (s.kind == Stmt::Kind::Class) {
      emit_class(ctx, s.class_decl, s.class_decl.name);
    }
  }
  {
    std::unordered_set<std::string> emitted_pkgs;
    for (const auto& kv : ctx.package_cpp) {
      if (emitted_pkgs.insert(kv.second).second) {
        emit_package(ctx, ctx.packages.at(kv.second));
      }
    }
  }

  // Emit top-level functions into a buffer so closure prelude can be placed first.
  std::ostringstream funcs_out;
  {
    std::ostringstream saved;
    saved << ctx.out.str();
    ctx.out.str("");
    ctx.out.clear();
    ctx.last_line = -1;
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
          cpp_params.push_back(local_sym(f.params[i]) + "__" + std::to_string(i));
        } else {
          cpp_params.push_back(local_sym(f.params[i]));
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
      ctx.local_vars.clear();
      for (const std::string& p : cpp_params) {
        // Only non-shadowed names (original names) are usable in the body.
        if (p.find("__") == std::string::npos) {
          ctx.env_params.push_back(p);
          ctx.local_vars.insert(p);
        }
      }
      const std::string tmp = ctx.fresh("t");
      ctx.out << "  id " << tmp << ";\n";
      emit_line(ctx, f.line);
      ctx.allow_return = true;
      ctx.import_binds = true;
      emit_body(ctx, f.body, tmp, false);
      ctx.allow_return = false;
      ctx.import_binds = false;
      ctx.env_params.clear();
      ctx.local_vars.clear();
      ctx.out << "}\n\n";
    }
    funcs_out << ctx.out.str();
    ctx.out.str("");
    ctx.out.clear();
    ctx.last_line = -1;
    ctx.out << saved.str();
  }

  ctx.out << ctx.prelude.str();
  // Reset prelude; top-level `my` / expr may emit more closures into it.
  ctx.prelude.str("");
  ctx.prelude.clear();
  // Forward-declare top-level functions so class field inits / methods can call them.
  for (const Stmt& s : program.stmts) {
    if (s.kind != Stmt::Kind::Func) {
      continue;
    }
    const FuncDecl& f = s.func_decl;
    ctx.out << "static id fn_" << f.name << "(";
    for (size_t i = 0; i < f.params.size(); ++i) {
      if (i) {
        ctx.out << ", ";
      }
      ctx.out << "id";
    }
    ctx.out << ");\n";
  }
  ctx.out << ctx.class_methods.str();
  ctx.class_methods.str("");
  ctx.class_methods.clear();
  ctx.out << funcs_out.str();

  // Emit main body first so any top-level lambdas land in prelude.
  std::ostringstream main_body;
  {
    std::ostringstream saved;
    saved << ctx.out.str();
    ctx.out.str("");
    ctx.out.clear();
    ctx.last_line = -1;
    ctx.out << "  runtime_package_init();\n";
    for (const auto& kv : ctx.package_cpp) {
      ctx.out << "  ensure_" << kv.second << "();\n";
    }
    // Top-level `my` bindings before class static init (lazy ensure on first use).
    ctx.env_params = ctx.toplevel_vars;
    for (const Stmt& s : program.stmts) {
      if (s.kind == Stmt::Kind::VarDecl) {
        ctx.current_class = nullptr;
        emit_line(ctx, s.line);
        if (s.expr) {
          emit_expr_top(ctx, *s.expr, local_sym(s.var_name));
        } else {
          ctx.out << "  " << local_sym(s.var_name) << " = null_id();\n";
        }
      }
    }
    for (const Stmt& s : program.stmts) {
      if (s.kind == Stmt::Kind::VarDecl) {
        continue;
      } else if (s.kind == Stmt::Kind::Expr) {
        ctx.current_class = nullptr;
        emit_line(ctx, s.line);
        // Bare function name → call
        if (s.expr->kind == Expr::Kind::Ident && ctx.functions.count(s.expr->text)) {
          ctx.out << "  (void)fn_" << s.expr->text << "();\n";
          continue;
        }
        if (s.expr->kind == Expr::Kind::Ident && ctx.imports.count(s.expr->text) &&
            ctx.imports[s.expr->text].kind == ImportKind::Func &&
            ctx.imports[s.expr->text].arity == 0) {
          const ImportBinding& b = ctx.imports[s.expr->text];
          ctx.out << "  (void)" << b.cpp << "__" << mangle_method(b.member, 0)
                  << "(as_id(&g_" << b.cpp << "), " << sel_expr(mangle_method(b.member, 0))
                  << ");\n";
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
    ctx.last_line = -1;
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
