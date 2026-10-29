constexpr bool f() {
  int a[1] = {0};
  int* p = a + 1;
  int& r = *p;                   // UB [expr.unary.op]/1: p points to no object
  (void)r;
  return true;
}
static_assert(f());
