struct A { int a; constexpr A(int x) : a(x) {} };
struct B : A {
  constexpr virtual int vf() const { return 1; }
  constexpr B() : A(vf()) {}     // UB [class.base.init]/16
};
constexpr int f() { B b; return b.a; }
static_assert(f() == 1);
