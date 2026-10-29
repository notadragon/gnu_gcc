struct T { int x = 0; };
constexpr int f() { T* p = new T; p->~T(); int r = p->x; delete p; return r; }
static_assert(f() == 0);
