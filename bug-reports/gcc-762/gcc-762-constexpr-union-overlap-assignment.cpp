union U { int i; short s; };
constexpr int f() {
  U u;
  u.s = 1;
  u.i = u.s;                     // UB [expr.assign]/8: inexact overlap
  return u.i;
}
static_assert(f() == 1);
