struct A { int a; };
struct B { int b; };
struct D : A, B {};
constexpr bool f() {
  int D::*pd = &A::a;
  int B::*pb = static_cast<int B::*>(pd);   // undefined: B does not contain A::a
  return pb != nullptr;
}
static_assert(f());
