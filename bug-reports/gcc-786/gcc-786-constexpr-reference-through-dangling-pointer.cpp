constexpr bool t() {
  int* p;
  { int y = 1; p = &y; }
  int& r = *p;   // UB [expr.unary.op]/1: p points to no object
  (void)r;
  return true;
}
static_assert(t());
