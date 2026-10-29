constexpr int f() {
  volatile int v = 1;
  return const_cast<int&>(v);   // undefined: volatile object accessed through non-volatile glvalue
}
static_assert(f() == 1);
