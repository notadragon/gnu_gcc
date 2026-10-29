struct A {
  constexpr virtual int f() const = 0;
  int v;
  constexpr A() : v(f()) {}      // UB [class.abstract]/6
};
constexpr int A::f() const { return 1; }
struct B : A { constexpr int f() const override { return 2; } };
constexpr int g() { B b; return b.v; }
static_assert(g() == 1);         // passes (GCC calls A::f)
