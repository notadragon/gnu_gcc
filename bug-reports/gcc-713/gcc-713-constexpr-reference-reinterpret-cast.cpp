float g = 1.0f;
constexpr int& r = reinterpret_cast<int&>(g);   // not a core constant expression
constexpr bool f() { float x = 1.0f; int& i = reinterpret_cast<int&>(x); (void)i; return true; }
static_assert(f());
