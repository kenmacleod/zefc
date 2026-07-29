// Generated from ScriptBench/nbody.zef (hand-maintained).
// Structure: Body accessible fields + Array + Double arithmetic via ZEFC_SITE sends.
// Inheritance (Jupiter/Saturn/…) flattened to Body ctors; loop indices are C++ ints.

#include <cstdarg>

#include "zefc/array_api.hpp"
#include "zefc/dispatch.hpp"
#include "zefc/double_api.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
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

static id Body__x_o(id self, int, ...) { return body<Body_>(self)->x; }
static id Body__y_o(id self, int, ...) { return body<Body_>(self)->y; }
static id Body__z_o(id self, int, ...) { return body<Body_>(self)->z; }
static id Body__vx_o(id self, int, ...) { return body<Body_>(self)->vx; }
static id Body__vy_o(id self, int, ...) { return body<Body_>(self)->vy; }
static id Body__vz_o(id self, int, ...) { return body<Body_>(self)->vz; }
static id Body__mass_o(id self, int, ...) { return body<Body_>(self)->mass; }

static id
Body__set_x_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  body<Body_>(self)->x = v;
  return null_id();
}

static id
Body__set_y_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  body<Body_>(self)->y = v;
  return null_id();
}

static id
Body__set_z_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  body<Body_>(self)->z = v;
  return null_id();
}

static id
Body__set_vx_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  body<Body_>(self)->vx = v;
  return null_id();
}

static id
Body__set_vy_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  body<Body_>(self)->vy = v;
  return null_id();
}

static id
Body__set_vz_o(id self, int selector, ...)
{
  (void)selector;
  std::va_list ap;
  va_start(ap, selector);
  id v = va_arg(ap, id);
  va_end(ap);
  body<Body_>(self)->vz = v;
  return null_id();
}

static void
ensure_body()
{
  if (Body_vtable) {
    return;
  }
  Body_vtable = vtable_create();
  vtable_set(Body_vtable, selector_intern("x_o"), Body__x_o);
  vtable_set(Body_vtable, selector_intern("y_o"), Body__y_o);
  vtable_set(Body_vtable, selector_intern("z_o"), Body__z_o);
  vtable_set(Body_vtable, selector_intern("vx_o"), Body__vx_o);
  vtable_set(Body_vtable, selector_intern("vy_o"), Body__vy_o);
  vtable_set(Body_vtable, selector_intern("vz_o"), Body__vz_o);
  vtable_set(Body_vtable, selector_intern("mass_o"), Body__mass_o);
  vtable_set(Body_vtable, selector_intern("set_x_o"), Body__set_x_o);
  vtable_set(Body_vtable, selector_intern("set_y_o"), Body__set_y_o);
  vtable_set(Body_vtable, selector_intern("set_z_o"), Body__set_z_o);
  vtable_set(Body_vtable, selector_intern("set_vx_o"), Body__set_vx_o);
  vtable_set(Body_vtable, selector_intern("set_vy_o"), Body__set_vy_o);
  vtable_set(Body_vtable, selector_intern("set_vz_o"), Body__set_vz_o);
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
  return ZEFC_SEND1(a, ZEFC_SITE("add_o"), b);
}

static id
dsub(id a, id b)
{
  return ZEFC_SEND1(a, ZEFC_SITE("sub_o"), b);
}

static id
dmul(id a, id b)
{
  return ZEFC_SEND1(a, ZEFC_SITE("mul_o"), b);
}

static id
ddiv(id a, id b)
{
  return ZEFC_SEND1(a, ZEFC_SITE("div_o"), b);
}

static id
dsqrt(id a)
{
  return ZEFC_SEND0(a, ZEFC_SITE("sqrt_o"));
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
    id b = Array__at(bodies, i);
    id mass = ZEFC_SEND0(b, ZEFC_SITE("mass_o"));
    px = dadd(px, dmul(ZEFC_SEND0(b, ZEFC_SITE("vx_o")), mass));
    py = dadd(py, dmul(ZEFC_SEND0(b, ZEFC_SITE("vy_o")), mass));
    pz = dadd(pz, dmul(ZEFC_SEND0(b, ZEFC_SITE("vz_o")), mass));
  }
  id sun = Array__at(bodies, 0);
  id solar = Double__from_f64(kSOLAR_MASS);
  (void)ZEFC_SEND1(sun, ZEFC_SITE("set_vx_o"), ddiv(dsub(Double__from_f64(0.), px), solar));
  (void)ZEFC_SEND1(sun, ZEFC_SITE("set_vy_o"), ddiv(dsub(Double__from_f64(0.), py), solar));
  (void)ZEFC_SEND1(sun, ZEFC_SITE("set_vz_o"), ddiv(dsub(Double__from_f64(0.), pz), solar));
}

static void
advance(id bodies, id dt)
{
  const int n = Array__size(bodies);
  for (int i = 0; i < n; ++i) {
    id bodyi = Array__at(bodies, i);
    id vxi = ZEFC_SEND0(bodyi, ZEFC_SITE("vx_o"));
    id vyi = ZEFC_SEND0(bodyi, ZEFC_SITE("vy_o"));
    id vzi = ZEFC_SEND0(bodyi, ZEFC_SITE("vz_o"));
    for (int j = i + 1; j < n; ++j) {
      id bodyj = Array__at(bodies, j);
      id dx = dsub(ZEFC_SEND0(bodyi, ZEFC_SITE("x_o")), ZEFC_SEND0(bodyj, ZEFC_SITE("x_o")));
      id dy = dsub(ZEFC_SEND0(bodyi, ZEFC_SITE("y_o")), ZEFC_SEND0(bodyj, ZEFC_SITE("y_o")));
      id dz = dsub(ZEFC_SEND0(bodyi, ZEFC_SITE("z_o")), ZEFC_SEND0(bodyj, ZEFC_SITE("z_o")));
      id d2 = dadd(dadd(dmul(dx, dx), dmul(dy, dy)), dmul(dz, dz));
      id mag = ddiv(dt, dmul(d2, dsqrt(d2)));
      id massj = ZEFC_SEND0(bodyj, ZEFC_SITE("mass_o"));
      vxi = dsub(vxi, dmul(dmul(dx, massj), mag));
      vyi = dsub(vyi, dmul(dmul(dy, massj), mag));
      vzi = dsub(vzi, dmul(dmul(dz, massj), mag));
      id massi = ZEFC_SEND0(bodyi, ZEFC_SITE("mass_o"));
      (void)ZEFC_SEND1(bodyj, ZEFC_SITE("set_vx_o"),
                       dadd(ZEFC_SEND0(bodyj, ZEFC_SITE("vx_o")), dmul(dmul(dx, massi), mag)));
      (void)ZEFC_SEND1(bodyj, ZEFC_SITE("set_vy_o"),
                       dadd(ZEFC_SEND0(bodyj, ZEFC_SITE("vy_o")), dmul(dmul(dy, massi), mag)));
      (void)ZEFC_SEND1(bodyj, ZEFC_SITE("set_vz_o"),
                       dadd(ZEFC_SEND0(bodyj, ZEFC_SITE("vz_o")), dmul(dmul(dz, massi), mag)));
    }
    (void)ZEFC_SEND1(bodyi, ZEFC_SITE("set_vx_o"), vxi);
    (void)ZEFC_SEND1(bodyi, ZEFC_SITE("set_vy_o"), vyi);
    (void)ZEFC_SEND1(bodyi, ZEFC_SITE("set_vz_o"), vzi);
  }
  for (int i = 0; i < n; ++i) {
    id b = Array__at(bodies, i);
    (void)ZEFC_SEND1(b, ZEFC_SITE("set_x_o"),
                     dadd(ZEFC_SEND0(b, ZEFC_SITE("x_o")), dmul(dt, ZEFC_SEND0(b, ZEFC_SITE("vx_o")))));
    (void)ZEFC_SEND1(b, ZEFC_SITE("set_y_o"),
                     dadd(ZEFC_SEND0(b, ZEFC_SITE("y_o")), dmul(dt, ZEFC_SEND0(b, ZEFC_SITE("vy_o")))));
    (void)ZEFC_SEND1(b, ZEFC_SITE("set_z_o"),
                     dadd(ZEFC_SEND0(b, ZEFC_SITE("z_o")), dmul(dt, ZEFC_SEND0(b, ZEFC_SITE("vz_o")))));
  }
}

static id
energy(id bodies)
{
  id e = Double__from_f64(0.);
  const int n = Array__size(bodies);
  for (int i = 0; i < n; ++i) {
    id bodyi = Array__at(bodies, i);
    id massi = ZEFC_SEND0(bodyi, ZEFC_SITE("mass_o"));
    id vxi = ZEFC_SEND0(bodyi, ZEFC_SITE("vx_o"));
    id vyi = ZEFC_SEND0(bodyi, ZEFC_SITE("vy_o"));
    id vzi = ZEFC_SEND0(bodyi, ZEFC_SITE("vz_o"));
    e = dadd(e,
             dmul(dmul(Double__from_f64(0.5), massi),
                  dadd(dadd(dmul(vxi, vxi), dmul(vyi, vyi)), dmul(vzi, vzi))));
    for (int j = i + 1; j < n; ++j) {
      id bodyj = Array__at(bodies, j);
      id dx = dsub(ZEFC_SEND0(bodyi, ZEFC_SITE("x_o")), ZEFC_SEND0(bodyj, ZEFC_SITE("x_o")));
      id dy = dsub(ZEFC_SEND0(bodyi, ZEFC_SITE("y_o")), ZEFC_SEND0(bodyj, ZEFC_SITE("y_o")));
      id dz = dsub(ZEFC_SEND0(bodyi, ZEFC_SITE("z_o")), ZEFC_SEND0(bodyj, ZEFC_SITE("z_o")));
      id distance = dsqrt(dadd(dadd(dmul(dx, dx), dmul(dy, dy)), dmul(dz, dz)));
      e = dsub(e, ddiv(dmul(massi, ZEFC_SEND0(bodyj, ZEFC_SITE("mass_o"))), distance));
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
