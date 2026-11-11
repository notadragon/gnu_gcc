constexpr int f() {
  int a[3] = {1, 2, 3};
  int* p = a + 4;          // undefined: beyond one past the end
  return *(p - 4);         // evaluated through the invalid pointer
}
static_assert(f() == 1);
