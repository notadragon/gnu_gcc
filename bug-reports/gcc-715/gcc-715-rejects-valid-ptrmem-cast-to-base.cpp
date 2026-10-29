struct Bb { int i; };
struct C : Bb { int m; };
constexpr int f() {
  int Bb::*p = static_cast<int Bb::*>(&C::m);  // OK [expr.static.cast]
  C c{};
  c.m = 5;
  return c.*p;                                  // OK: c contains C::m
}
static_assert(f() == 5);
