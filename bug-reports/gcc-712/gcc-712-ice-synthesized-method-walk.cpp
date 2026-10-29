struct B { virtual ~B(); };
struct D : virtual B {};
constexpr int t() { D* d = new D(); delete d; return 0; }
static_assert(t() == 0);
