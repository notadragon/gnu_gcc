struct B { virtual constexpr ~B() {} };
struct D : B {};
constexpr int t(int x) {
  B bs[1];
  B* bp = bs + x;
  D* dp = dynamic_cast<D*>(bp);  // UB [expr.dynamic.cast]/7 for x == 1
  return dp == nullptr ? -1 : 0;
}
static_assert(t(1) == -1);
