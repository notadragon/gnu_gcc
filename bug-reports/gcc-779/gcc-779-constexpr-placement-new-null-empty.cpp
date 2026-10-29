#include <new>
struct E {};
constexpr bool f() { E* p = nullptr; new (p) E; return true; }   // UB [expr.new]
static_assert(f());
