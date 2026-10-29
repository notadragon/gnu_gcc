struct E {};
constexpr bool f() { E a[1]; E e; e = a[1]; return true; }   // UB [expr.unary.op]/1
static_assert(f());
