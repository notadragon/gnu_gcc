#include <memory>
struct B { int x; };
constexpr int f() {
  const B b{1};
  std::construct_at(const_cast<B*>(&b), 2);  // undefined: new object in a const object's storage
  return b.x;
}
static_assert(f() == 2);
