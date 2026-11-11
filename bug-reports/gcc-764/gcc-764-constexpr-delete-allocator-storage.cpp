#include <memory>
constexpr bool f() {
  std::allocator<int> a;
  int* p = a.allocate(1);
  delete p;                 // undefined: not from a non-array new-expression
  return true;
}
static_assert(f());
