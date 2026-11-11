constexpr int& id(int& r) { return r; }
constexpr bool f() { int& r = id(r); (void)r; return true; }   // undefined
static_assert(f());
