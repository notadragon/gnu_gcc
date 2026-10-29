constexpr bool f() {
  int a[2]{};
  void* v = a + 1;
  unsigned* p = static_cast<unsigned*>(v);
  return p;
}
static_assert(f());
