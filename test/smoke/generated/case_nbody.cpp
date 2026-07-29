// Generated from ScriptBench/nbody.zef (hand-maintained).
// Structure: Body + Array + Double arith. Inheritance flattened to Body ctors.
//
// Field access: monomorphic lowering — known Body receiver uses direct struct
// loads/stores (what a future compiler emits when the class is fixed). Get/set
// methods stay on the vtable for the accessible ABI / polymorphic sends.
// Double ops use ZEFC_SEL_* + immediate short-circuit.

#include <cstdarg>

#include "zefc/array_api.hpp"
#include "zefc/dispatch.hpp"
#include "zefc/double_api.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/known_selectors.hpp"
#include "zefc/runtime.hpp"
#include "smoke_cases.hpp"

namespace zefc {
namespace smoke {

namespace {

constexpr double kPI = 3.14159265358979323;
constexpr double kSOLAR_MASS = 4. * kPI * kPI;
constexpr double kDAYS_PER_YEAR = 365.24;

struct Body_ {
  VTable* isa_;
  id x;
  id y;
  id z;
  id vx;
  id vy;
  id vz;
  id mass;
};

static VTable* Body_vtable = nullptr;

// --- Accessible ABI (vtable); hot path uses BODY_* below instead ---

static id Body__x_o(id self, int, ...) { return body<Body_>(self)->x; }
static id Body__y_o(id self, int, ...) { return body<Body_>(self)->y; }
static id Body__z_o(id self, int, ...) { return body<Body_>(self)->z; }
static id Body__vx_o(id self, int, ...) { return body<Body_>(self)->vx; }
static id Body__vy_o(id self, int, ...) { return body<Body_>(self)->vy; }
static id Body__vz_o(id self, int, ...) { return body<Body_>(self)->vz; }
static id Body__mass_o(id self, int, ...) { return body<Body_>(self)->mass; }

static id
Body__set_field(id /*self*/, id* slot, id v)
{
  *slot = v;
  return null_id();
}

static id
Body__set_x_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  return Body__set_field(self, &body<Body_>(self)->x, v);
}

static id
Body__set_y_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  return Body__set_field(self, &body<Body_>(self)->y, v);
}

static id
Body__set_z_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  return Body__set_field(self, &body<Body_>(self)->z, v);
}

static id
Body__set_vx_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  return Body__set_field(self, &body<Body_>(self)->vx, v);
}

static id
Body__set_vy_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  return Body__set_field(self, &body<Body_>(self)->vy, v);
}

static id
Body__set_vz_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  return Body__set_field(self, &body<Body_>(self)->vz, v);
}

// Monomorphic field access for a known Body* receiver.
static inline Body_* as_body(id obj) { return body<Body_>(obj); }

static void
ensure_body()
{
  if (Body_vtable) {
    return;
  }
  Body_vtable = vtable_create();
  vtable_set(Body_vtable, ZEFC_SEL_x_o, Body__x_o);
  vtable_set(Body_vtable, ZEFC_SEL_y_o, Body__y_o);
  vtable_set(Body_vtable, ZEFC_SEL_z_o, Body__z_o);
  vtable_set(Body_vtable, ZEFC_SEL_vx_o, Body__vx_o);
  vtable_set(Body_vtable, ZEFC_SEL_vy_o, Body__vy_o);
  vtable_set(Body_vtable, ZEFC_SEL_vz_o, Body__vz_o);
  vtable_set(Body_vtable, ZEFC_SEL_mass_o, Body__mass_o);
  vtable_set(Body_vtable, ZEFC_SEL_set_x_o, Body__set_x_o);
  vtable_set(Body_vtable, ZEFC_SEL_set_y_o, Body__set_y_o);
  vtable_set(Body_vtable, ZEFC_SEL_set_z_o, Body__set_z_o);
  vtable_set(Body_vtable, ZEFC_SEL_set_vx_o, Body__set_vx_o);
  vtable_set(Body_vtable, ZEFC_SEL_set_vy_o, Body__set_vy_o);
  vtable_set(Body_vtable, ZEFC_SEL_set_vz_o, Body__set_vz_o);
  selector_sites_patch();
}

static id
Body__new(double inX, double inY, double inZ, double inVX, double inVY, double inVZ, double inMass)
{
  ensure_body();
  Body_* o = alloc<Body_>();
  o->isa_ = Body_vtable;
  o->x = Double__from_f64(inX);
  o->y = Double__from_f64(inY);
  o->z = Double__from_f64(inZ);
  o->vx = Double__from_f64(inVX);
  o->vy = Double__from_f64(inVY);
  o->vz = Double__from_f64(inVZ);
  o->mass = Double__from_f64(inMass);
  return as_id(o);
}

static id
dadd(id a, id b)
{
  return ZEFC_SEND1(a, ZEFC_SEL_add_o, b);
}

static id
dsub(id a, id b)
{
  return ZEFC_SEND1(a, ZEFC_SEL_sub_o, b);
}

static id
dmul(id a, id b)
{
  return ZEFC_SEND1(a, ZEFC_SEL_mul_o, b);
}

static id
ddiv(id a, id b)
{
  return ZEFC_SEND1(a, ZEFC_SEL_div_o, b);
}

static id
dsqrt(id a)
{
  return ZEFC_SEND0(a, ZEFC_SEL_sqrt_o);
}

static id
Sun__new()
{
  return Body__new(0., 0., 0., 0., 0., 0., kSOLAR_MASS);
}

static id
Jupiter__new()
{
  return Body__new(
    4.84143144246472090e+00,
    -1.16032004402742839e+00,
    -1.03622044471123109e-01,
    1.66007664274403694e-03 * kDAYS_PER_YEAR,
    7.69901118419740425e-03 * kDAYS_PER_YEAR,
    -6.90460016972063023e-05 * kDAYS_PER_YEAR,
    9.54791938424326609e-04 * kSOLAR_MASS);
}

static id
Saturn__new()
{
  return Body__new(
    8.34336671824457987e+00,
    4.12479856412430479e+00,
    -4.03523417114321381e-01,
    -2.76742510726862411e-03 * kDAYS_PER_YEAR,
    4.99852801234917238e-03 * kDAYS_PER_YEAR,
    2.30417297573763929e-05 * kDAYS_PER_YEAR,
    2.85885980666130812e-04 * kSOLAR_MASS);
}

static id
Uranus__new()
{
  return Body__new(
    1.28943695621391310e+01,
    -1.51111514016986312e+01,
    -2.23307578892655734e-01,
    2.96460137564761618e-03 * kDAYS_PER_YEAR,
    2.37847173959480950e-03 * kDAYS_PER_YEAR,
    -2.96589568540237556e-05 * kDAYS_PER_YEAR,
    4.36624404335156298e-05 * kSOLAR_MASS);
}

static id
Neptune__new()
{
  return Body__new(
    1.53796971148509165e+01,
    -2.59193146099879641e+01,
    1.79258772950371181e-01,
    2.68067772490389322e-03 * kDAYS_PER_YEAR,
    1.62824170038242295e-03 * kDAYS_PER_YEAR,
    -9.51592254519715870e-05 * kDAYS_PER_YEAR,
    5.15138902046611451e-05 * kSOLAR_MASS);
}

static void
offsetMomentum(id bodies)
{
  id px = Double__from_f64(0.);
  id py = Double__from_f64(0.);
  id pz = Double__from_f64(0.);
  const int n = Array__size(bodies);
  for (int i = 0; i < n; ++i) {
    Body_* b = as_body(Array__at(bodies, i));
    id mass = b->mass;
    px = dadd(px, dmul(b->vx, mass));
    py = dadd(py, dmul(b->vy, mass));
    pz = dadd(pz, dmul(b->vz, mass));
  }
  Body_* sun = as_body(Array__at(bodies, 0));
  id solar = Double__from_f64(kSOLAR_MASS);
  sun->vx = ddiv(dsub(Double__from_f64(0.), px), solar);
  sun->vy = ddiv(dsub(Double__from_f64(0.), py), solar);
  sun->vz = ddiv(dsub(Double__from_f64(0.), pz), solar);
}

static void
advance(id bodies, id dt)
{
  const int n = Array__size(bodies);
  for (int i = 0; i < n; ++i) {
    Body_* bodyi = as_body(Array__at(bodies, i));
    id vxi = bodyi->vx;
    id vyi = bodyi->vy;
    id vzi = bodyi->vz;
    for (int j = i + 1; j < n; ++j) {
      Body_* bodyj = as_body(Array__at(bodies, j));
      id dx = dsub(bodyi->x, bodyj->x);
      id dy = dsub(bodyi->y, bodyj->y);
      id dz = dsub(bodyi->z, bodyj->z);
      id d2 = dadd(dadd(dmul(dx, dx), dmul(dy, dy)), dmul(dz, dz));
      id mag = ddiv(dt, dmul(d2, dsqrt(d2)));
      id massj = bodyj->mass;
      vxi = dsub(vxi, dmul(dmul(dx, massj), mag));
      vyi = dsub(vyi, dmul(dmul(dy, massj), mag));
      vzi = dsub(vzi, dmul(dmul(dz, massj), mag));
      id massi = bodyi->mass;
      bodyj->vx = dadd(bodyj->vx, dmul(dmul(dx, massi), mag));
      bodyj->vy = dadd(bodyj->vy, dmul(dmul(dy, massi), mag));
      bodyj->vz = dadd(bodyj->vz, dmul(dmul(dz, massi), mag));
    }
    bodyi->vx = vxi;
    bodyi->vy = vyi;
    bodyi->vz = vzi;
  }
  for (int i = 0; i < n; ++i) {
    Body_* b = as_body(Array__at(bodies, i));
    b->x = dadd(b->x, dmul(dt, b->vx));
    b->y = dadd(b->y, dmul(dt, b->vy));
    b->z = dadd(b->z, dmul(dt, b->vz));
  }
}

static id
energy(id bodies)
{
  id e = Double__from_f64(0.);
  const int n = Array__size(bodies);
  for (int i = 0; i < n; ++i) {
    Body_* bodyi = as_body(Array__at(bodies, i));
    id massi = bodyi->mass;
    id vxi = bodyi->vx;
    id vyi = bodyi->vy;
    id vzi = bodyi->vz;
    e = dadd(e,
             dmul(dmul(Double__from_f64(0.5), massi),
                  dadd(dadd(dmul(vxi, vxi), dmul(vyi, vyi)), dmul(vzi, vzi))));
    for (int j = i + 1; j < n; ++j) {
      Body_* bodyj = as_body(Array__at(bodies, j));
      id dx = dsub(bodyi->x, bodyj->x);
      id dy = dsub(bodyi->y, bodyj->y);
      id dz = dsub(bodyi->z, bodyj->z);
      id distance = dsqrt(dadd(dadd(dmul(dx, dx), dmul(dy, dy)), dmul(dz, dz)));
      e = dsub(e, ddiv(dmul(massi, bodyj->mass), distance));
    }
  }
  return e;
}

} // namespace

void
smoke_nbody()
{
  id bodies = Array__new();
  (void)Array__push(bodies, Sun__new());
  (void)Array__push(bodies, Jupiter__new());
  (void)Array__push(bodies, Saturn__new());
  (void)Array__push(bodies, Uranus__new());
  (void)Array__push(bodies, Neptune__new());

  offsetMomentum(bodies);
  println(energy(bodies));
  id dt = Double__from_f64(0.01);
  constexpr int n = 5000;
  for (int i = 0; i < n; ++i) {
    advance(bodies, dt);
  }
  println(energy(bodies));
}

} // namespace smoke
} // namespace zefc
