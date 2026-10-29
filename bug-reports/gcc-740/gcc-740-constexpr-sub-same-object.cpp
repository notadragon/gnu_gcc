struct M { int a, b; };
constexpr long f() { M m{}; return &m.b - &m.a; }   // undefined: not elements of one array
static_assert(f() == 1);
