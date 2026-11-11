constexpr bool f() { int a[1] = {0}; int* p = &*(a + 1); return p != nullptr; }   // UB [expr.unary.op]/1
static_assert(f());
