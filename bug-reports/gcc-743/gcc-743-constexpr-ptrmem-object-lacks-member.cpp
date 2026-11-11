struct B { int dummy = 0; };
struct D : B { int x = 0; constexpr int h() const { return 2; } };
constexpr int f() {
  int (B::*pb)() const = static_cast<int (B::*)() const>(&D::h);  // OK
  B b;
  return (b.*pb)();          // undefined: b does not contain D::h
}
static_assert(f() == 2);
