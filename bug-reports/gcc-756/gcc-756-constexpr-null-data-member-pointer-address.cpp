struct S { int m = 1; };
constexpr bool f() {
  int S::*pm = nullptr;
  S s;
  int* p = &(s.*pm);             // UB [expr.mptr.oper]/6
  return p != nullptr;
}
static_assert(f());
