// Generated from ScriptBench/splay.zef (hand-maintained).
// Fair transpile shape: tree methods via sends; node/payload fields via ZEFC_IC_*.
// RNG is C++ uint32 (not a dispatch probe). Closures for exportKeys traverse.

#include "zefc/array_api.hpp"
#include "zefc/dispatch.hpp"
#include "zefc/error.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/known_selectors.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

#include <cstdint>

namespace zefc {
namespace smoke {

namespace {

constexpr int kSplayTreeSize = 8000;
constexpr int kSplayTreeModifications = 80;
constexpr int kSplayTreePayloadDepth = 5;

static id g_splayTree = nullptr;
static uint32_t g_randomState = 42;

static bool
falsy(id v)
{
  if (v == null_id()) {
    return true;
  }
  if (id_is_int32(v)) {
    return Int__to_i64(v) == 0;
  }
  return false;
}

static bool
truthy(id v)
{
  return !falsy(v);
}

static long long
ikey(id v)
{
  return Int__to_i64(v);
}

// --- selectors / vtables ---

static int sel_insert = 0;
static int sel_remove = 0;
static int sel_find = 0;
static int sel_findMax = 0;
static int sel_findGreatestLessThan = 0;
static int sel_exportKeys = 0;
static int sel_splay = 0;
static int sel_isEmpty = 0;
static int sel_key = 0;
static int sel_value = 0;
static int sel_left = 0;
static int sel_set_left = 0;
static int sel_right = 0;
static int sel_set_right = 0;
static int sel_traverse = 0;
static int sel_call = 0;
static int sel_root = 0;
static int sel_set_root = 0;

static VTable* Leaf_vtable = nullptr;
static VTable* Inner_vtable = nullptr;
static VTable* Node_vtable = nullptr;
static VTable* Tree_vtable = nullptr;
static VTable* PushKey_vtable = nullptr;

struct Leaf_ {
  VTable* isa_;
  id array;
  id string;
};

struct Inner_ {
  VTable* isa_;
  id left;
  id right;
};

struct Node_ {
  VTable* isa_;
  id key;
  id value;
  id left;
  id right;
};

struct Tree_ {
  VTable* isa_;
  id root;
};

struct PushKey_ {
  VTable* isa_;
  id result;
};

static id Leaf_get_array(id self) { return body<Leaf_>(self)->array; }
static id Leaf_get_string(id self) { return body<Leaf_>(self)->string; }
static id Inner_get_left(id self) { return body<Inner_>(self)->left; }
static id Inner_get_right(id self) { return body<Inner_>(self)->right; }

static id Node_get_key(id self) { return body<Node_>(self)->key; }
static id Node_get_value(id self) { return body<Node_>(self)->value; }
static id Node_get_left(id self) { return body<Node_>(self)->left; }
static id Node_get_right(id self) { return body<Node_>(self)->right; }
static id Node_set_left(id self, id v)
{
  body<Node_>(self)->left = v;
  return null_id();
}
static id Node_set_right(id self, id v)
{
  body<Node_>(self)->right = v;
  return null_id();
}

static id Tree_get_root(id self) { return body<Tree_>(self)->root; }
static id Tree_set_root(id self, id v)
{
  body<Tree_>(self)->root = v;
  return null_id();
}

static id Node__key_o(id self, int, ...) { return Node_get_key(self); }
static id Node__value_o(id self, int, ...) { return Node_get_value(self); }
static id Node__left_o(id self, int, ...) { return Node_get_left(self); }
static id Node__right_o(id self, int, ...) { return Node_get_right(self); }
static id Node__set_left_o(id self, int, id v) { return Node_set_left(self, v); }
static id Node__set_right_o(id self, int, id v) { return Node_set_right(self, v); }

static id Tree__root_o(id self, int, ...) { return Tree_get_root(self); }
static id Tree__set_root_o(id self, int, id v) { return Tree_set_root(self, v); }

static id Leaf__array_o(id self, int, ...) { return Leaf_get_array(self); }
static id Leaf__string_o(id self, int, ...) { return Leaf_get_string(self); }
static id Inner__left_o(id self, int, ...) { return Inner_get_left(self); }
static id Inner__right_o(id self, int, ...) { return Inner_get_right(self); }

static id Node__new(id key, id value);
static id Tree__splay_o(id self, int selector, id key);
static id Tree__isEmpty_o(id self, int selector, ...);
static id Tree__insert_oo(id self, int selector, id key, id value);
static id Tree__remove_o(id self, int selector, id key);
static id Tree__find_o(id self, int selector, id key);
static id Tree__findMax_o(id self, int selector, id opt_start);
static id Tree__findGreatestLessThan_o(id self, int selector, id key);
static id Tree__exportKeys_o(id self, int selector, ...);

static id
PushKey__call_o(id self, int selector, id node)
{
  (void)selector;
  id result = body<PushKey_>(self)->result;
  (void)Array__push(result, ZEFC_IC_GET(node, sel_key));
  return null_id();
}

static id
Node__traverse_o(id self, int selector, id func)
{
  (void)selector;
  id current = self;
  while (truthy(current)) {
    id left = ZEFC_IC_GET(current, sel_left);
    if (truthy(left)) {
      (void)ZEFC_SEND1(left, sel_traverse, func);
    }
    (void)ZEFC_SEND1(func, sel_call, current);
    current = ZEFC_IC_GET(current, sel_right);
  }
  return null_id();
}

static void
ensure_runtime()
{
  if (Tree_vtable) {
    return;
  }
  sel_insert = selector_intern("insert_oo");
  sel_remove = selector_intern("remove_o");
  sel_find = selector_intern("find_o");
  sel_findMax = selector_intern("findMax_o");
  sel_findGreatestLessThan = selector_intern("findGreatestLessThan_o");
  sel_exportKeys = selector_intern("exportKeys_o");
  sel_splay = selector_intern("splay_o");
  sel_isEmpty = selector_intern("isEmpty_o");
  sel_key = selector_intern("key_o");
  sel_value = selector_intern("value_o");
  sel_left = selector_intern("left_o");
  sel_set_left = selector_intern("set_left_o");
  sel_right = selector_intern("right_o");
  sel_set_right = selector_intern("set_right_o");
  sel_traverse = selector_intern("traverse_o");
  sel_call = selector_intern("call_o");
  sel_root = selector_intern("root_o");
  sel_set_root = selector_intern("set_root_o");

  Leaf_vtable = vtable_create();
  Inner_vtable = vtable_create();
  Node_vtable = vtable_create();
  Tree_vtable = vtable_create();
  PushKey_vtable = vtable_create();

  vtable_set(Leaf_vtable, selector_intern("array_o"), Leaf__array_o);
  vtable_set(Leaf_vtable, selector_intern("string_o"), Leaf__string_o);
  field_register_get(Leaf_vtable, selector_intern("array_o"), Leaf_get_array);
  field_register_get(Leaf_vtable, selector_intern("string_o"), Leaf_get_string);

  vtable_set(Inner_vtable, sel_left, Inner__left_o);
  vtable_set(Inner_vtable, sel_right, Inner__right_o);
  field_register_get(Inner_vtable, sel_left, Inner_get_left);
  field_register_get(Inner_vtable, sel_right, Inner_get_right);

  vtable_set(Node_vtable, sel_key, Node__key_o);
  vtable_set(Node_vtable, sel_value, Node__value_o);
  vtable_set(Node_vtable, sel_left, Node__left_o);
  vtable_set(Node_vtable, sel_right, Node__right_o);
  vtable_set(Node_vtable, sel_set_left, Node__set_left_o);
  vtable_set(Node_vtable, sel_set_right, Node__set_right_o);
  vtable_set(Node_vtable, sel_traverse, Node__traverse_o);
  field_register_get(Node_vtable, sel_key, Node_get_key);
  field_register_get(Node_vtable, sel_value, Node_get_value);
  field_register_get(Node_vtable, sel_left, Node_get_left);
  field_register_get(Node_vtable, sel_right, Node_get_right);
  field_register_set(Node_vtable, sel_set_left, Node_set_left);
  field_register_set(Node_vtable, sel_set_right, Node_set_right);

  vtable_set(Tree_vtable, sel_root, Tree__root_o);
  vtable_set(Tree_vtable, sel_set_root, Tree__set_root_o);
  vtable_set(Tree_vtable, sel_isEmpty, Tree__isEmpty_o);
  vtable_set(Tree_vtable, sel_insert, Tree__insert_oo);
  vtable_set(Tree_vtable, sel_remove, Tree__remove_o);
  vtable_set(Tree_vtable, sel_find, Tree__find_o);
  vtable_set(Tree_vtable, sel_findMax, Tree__findMax_o);
  vtable_set(Tree_vtable, sel_findGreatestLessThan, Tree__findGreatestLessThan_o);
  vtable_set(Tree_vtable, sel_exportKeys, Tree__exportKeys_o);
  vtable_set(Tree_vtable, sel_splay, Tree__splay_o);
  field_register_get(Tree_vtable, sel_root, Tree_get_root);
  field_register_set(Tree_vtable, sel_set_root, Tree_set_root);

  vtable_set(PushKey_vtable, sel_call, PushKey__call_o);
  selector_sites_patch();
}

static id
Leaf__new(id array, id string)
{
  ensure_runtime();
  Leaf_* o = alloc<Leaf_>();
  o->isa_ = Leaf_vtable;
  o->array = array;
  o->string = string;
  return as_id(o);
}

static id
Inner__new(id left, id right)
{
  ensure_runtime();
  Inner_* o = alloc<Inner_>();
  o->isa_ = Inner_vtable;
  o->left = left;
  o->right = right;
  return as_id(o);
}

static id
Node__new(id key, id value)
{
  ensure_runtime();
  Node_* o = alloc<Node_>();
  o->isa_ = Node_vtable;
  o->key = key;
  o->value = value;
  o->left = null_id();
  o->right = null_id();
  return as_id(o);
}

static id
Tree__new()
{
  ensure_runtime();
  Tree_* o = alloc<Tree_>();
  o->isa_ = Tree_vtable;
  o->root = null_id();
  return as_id(o);
}

static id
make_push_key(id result)
{
  ensure_runtime();
  PushKey_* o = alloc<PushKey_>();
  o->isa_ = PushKey_vtable;
  o->result = result;
  return as_id(o);
}

static id
GeneratePayloadTree(int depth, id tag)
{
  if (depth == 0) {
    id s = String__from_utf8("String for key ");
    s = send(s, ZEFC_SEL_add_o, ZEFC_SEND0(tag, ZEFC_SEL_toString_o));
    s = send(s, ZEFC_SEL_add_o, String__from_utf8(" in leaf node"));
    return Leaf__new(Array__from_ints({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}), s);
  }
  return Inner__new(GeneratePayloadTree(depth - 1, tag), GeneratePayloadTree(depth - 1, tag));
}

static id
GenerateKey()
{
  g_randomState ^= (g_randomState << 13);
  g_randomState ^= (g_randomState >> 17);
  g_randomState ^= (g_randomState << 5);
  return Int__from_i64(static_cast<long long>(g_randomState));
}

static id
Tree__isEmpty_o(id self, int selector, ...)
{
  (void)selector;
  return falsy(ZEFC_IC_GET(self, sel_root)) ? Int__from_i64(1) : Int__from_i64(0);
}

static id
Tree__splay_o(id self, int selector, id key)
{
  (void)selector;
  if (truthy(ZEFC_SEND0(self, sel_isEmpty))) {
    return null_id();
  }
  id dummy = Node__new(Int__from_i64(0), null_id());
  id left = dummy;
  id right = dummy;
  id current = ZEFC_IC_GET(self, sel_root);
  for (;;) {
    if (ikey(key) < ikey(ZEFC_IC_GET(current, sel_key))) {
      if (falsy(ZEFC_IC_GET(current, sel_left))) {
        break;
      }
      if (ikey(key) < ikey(ZEFC_IC_GET(ZEFC_IC_GET(current, sel_left), sel_key))) {
        id tmp = ZEFC_IC_GET(current, sel_left);
        (void)ZEFC_IC_SET(current, sel_set_left, ZEFC_IC_GET(tmp, sel_right));
        (void)ZEFC_IC_SET(tmp, sel_set_right, current);
        current = tmp;
        if (falsy(ZEFC_IC_GET(current, sel_left))) {
          break;
        }
      }
      (void)ZEFC_IC_SET(right, sel_set_left, current);
      right = current;
      current = ZEFC_IC_GET(current, sel_left);
    } else if (ikey(key) > ikey(ZEFC_IC_GET(current, sel_key))) {
      if (falsy(ZEFC_IC_GET(current, sel_right))) {
        break;
      }
      if (ikey(key) > ikey(ZEFC_IC_GET(ZEFC_IC_GET(current, sel_right), sel_key))) {
        id tmp = ZEFC_IC_GET(current, sel_right);
        (void)ZEFC_IC_SET(current, sel_set_right, ZEFC_IC_GET(tmp, sel_left));
        (void)ZEFC_IC_SET(tmp, sel_set_left, current);
        current = tmp;
        if (falsy(ZEFC_IC_GET(current, sel_right))) {
          break;
        }
      }
      (void)ZEFC_IC_SET(left, sel_set_right, current);
      left = current;
      current = ZEFC_IC_GET(current, sel_right);
    } else {
      break;
    }
  }
  (void)ZEFC_IC_SET(left, sel_set_right, ZEFC_IC_GET(current, sel_left));
  (void)ZEFC_IC_SET(right, sel_set_left, ZEFC_IC_GET(current, sel_right));
  (void)ZEFC_IC_SET(current, sel_set_left, ZEFC_IC_GET(dummy, sel_right));
  (void)ZEFC_IC_SET(current, sel_set_right, ZEFC_IC_GET(dummy, sel_left));
  (void)ZEFC_IC_SET(self, sel_set_root, current);
  return null_id();
}

static id
Tree__insert_oo(id self, int selector, id key, id value)
{
  (void)selector;
  if (truthy(ZEFC_SEND0(self, sel_isEmpty))) {
    (void)ZEFC_IC_SET(self, sel_set_root, Node__new(key, value));
    return null_id();
  }
  (void)ZEFC_SEND1(self, sel_splay, key);
  id root = ZEFC_IC_GET(self, sel_root);
  if (ikey(ZEFC_IC_GET(root, sel_key)) == ikey(key)) {
    return null_id();
  }
  id node = Node__new(key, value);
  if (ikey(key) > ikey(ZEFC_IC_GET(root, sel_key))) {
    (void)ZEFC_IC_SET(node, sel_set_left, root);
    (void)ZEFC_IC_SET(node, sel_set_right, ZEFC_IC_GET(root, sel_right));
    (void)ZEFC_IC_SET(root, sel_set_right, null_id());
  } else {
    (void)ZEFC_IC_SET(node, sel_set_right, root);
    (void)ZEFC_IC_SET(node, sel_set_left, ZEFC_IC_GET(root, sel_left));
    (void)ZEFC_IC_SET(root, sel_set_left, null_id());
  }
  (void)ZEFC_IC_SET(self, sel_set_root, node);
  return null_id();
}

static id
Tree__remove_o(id self, int selector, id key)
{
  (void)selector;
  if (truthy(ZEFC_SEND0(self, sel_isEmpty))) {
    zefc_error("Key not found (empty tree)");
  }
  (void)ZEFC_SEND1(self, sel_splay, key);
  id root = ZEFC_IC_GET(self, sel_root);
  if (ikey(ZEFC_IC_GET(root, sel_key)) != ikey(key)) {
    zefc_error("Key not found");
  }
  id removed = root;
  if (falsy(ZEFC_IC_GET(root, sel_left))) {
    (void)ZEFC_IC_SET(self, sel_set_root, ZEFC_IC_GET(root, sel_right));
  } else {
    id right = ZEFC_IC_GET(root, sel_right);
    (void)ZEFC_IC_SET(self, sel_set_root, ZEFC_IC_GET(root, sel_left));
    (void)ZEFC_SEND1(self, sel_splay, key);
    (void)ZEFC_IC_SET(ZEFC_IC_GET(self, sel_root), sel_set_right, right);
  }
  return removed;
}

static id
Tree__find_o(id self, int selector, id key)
{
  (void)selector;
  if (truthy(ZEFC_SEND0(self, sel_isEmpty))) {
    return null_id();
  }
  (void)ZEFC_SEND1(self, sel_splay, key);
  id root = ZEFC_IC_GET(self, sel_root);
  if (ikey(ZEFC_IC_GET(root, sel_key)) == ikey(key)) {
    return root;
  }
  return null_id();
}

static id
Tree__findMax_o(id self, int selector, id opt_start)
{
  (void)selector;
  if (truthy(ZEFC_SEND0(self, sel_isEmpty))) {
    return null_id();
  }
  id current = truthy(opt_start) ? opt_start : ZEFC_IC_GET(self, sel_root);
  while (truthy(ZEFC_IC_GET(current, sel_right))) {
    current = ZEFC_IC_GET(current, sel_right);
  }
  return current;
}

static id
Tree__findGreatestLessThan_o(id self, int selector, id key)
{
  (void)selector;
  if (truthy(ZEFC_SEND0(self, sel_isEmpty))) {
    return null_id();
  }
  (void)ZEFC_SEND1(self, sel_splay, key);
  id root = ZEFC_IC_GET(self, sel_root);
  if (ikey(ZEFC_IC_GET(root, sel_key)) < ikey(key)) {
    return root;
  }
  if (truthy(ZEFC_IC_GET(root, sel_left))) {
    return ZEFC_SEND1(self, sel_findMax, ZEFC_IC_GET(root, sel_left));
  }
  return null_id();
}

static id
Tree__exportKeys_o(id self, int selector, ...)
{
  (void)selector;
  id result = Array__new();
  if (!truthy(ZEFC_SEND0(self, sel_isEmpty))) {
    (void)ZEFC_SEND1(ZEFC_IC_GET(self, sel_root), sel_traverse, make_push_key(result));
  }
  return result;
}

static id
InsertNewNode()
{
  id key = GenerateKey();
  while (truthy(ZEFC_SEND1(g_splayTree, sel_find, key))) {
    key = GenerateKey();
  }
  id payload = GeneratePayloadTree(kSplayTreePayloadDepth, key);
  (void)ZEFC_SEND2(g_splayTree, sel_insert, key, payload);
  return key;
}

static void
SplaySetup()
{
  println(String__from_utf8("Setting up splay tree"));
  g_splayTree = Tree__new();
  for (int i = 0; i < kSplayTreeSize; ++i) {
    (void)InsertNewNode();
  }
}

static void
SplayTearDown()
{
  println(String__from_utf8("Tearing down splay tree"));
  id keys = ZEFC_SEND0(g_splayTree, sel_exportKeys);
  g_splayTree = null_id();
  const int length = Array__size(keys);
  if (length != kSplayTreeSize) {
    println(String__from_utf8("Splay tree has wrong size"));
  }
  for (int i = 0; i < length - 1; ++i) {
    if (ikey(Array__at(keys, i)) >= ikey(Array__at(keys, i + 1))) {
      println(String__from_utf8("Splay tree not sorted"));
    }
  }
}

static void
SplayRun()
{
  println(String__from_utf8("Running"));
  for (int i = 0; i < kSplayTreeModifications; ++i) {
    id key = InsertNewNode();
    id greatest = ZEFC_SEND1(g_splayTree, sel_findGreatestLessThan, key);
    if (falsy(greatest)) {
      (void)ZEFC_SEND1(g_splayTree, sel_remove, key);
    } else {
      (void)ZEFC_SEND1(g_splayTree, sel_remove, ZEFC_IC_GET(greatest, sel_key));
    }
  }
}

} // namespace

void
smoke_splay()
{
  ensure_runtime();
  SplaySetup();
  for (int i = 0; i < 32; ++i) {
    SplayRun();
  }
  SplayTearDown();
}

} // namespace smoke
} // namespace zefc
