struct A { int a; constexpr A(int x) : a(x) {} };
struct B : A {
  constexpr int g() const { return 1; }
  constexpr B() : A(g()) {}     // UB [class.base.init]/16
};
static_assert((B{}, true));
