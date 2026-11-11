#include <memory>
#include <typeinfo>
struct B { constexpr virtual ~B() {} };
struct D : B {};
constexpr int t() {
  D d;
  B& br = d;
  d.~D();
  (void) typeid(br);              // undefined: d is outside its lifetime
  std::construct_at(&d);
  std::allocator<D> a;
  D* p = a.allocate(1);
  B* bp = std::construct_at(p);
  std::destroy_at(p);
  (void) dynamic_cast<D*>(bp);    // undefined: *p is outside its lifetime
  a.deallocate(p, 1);
  return 1;
}
static_assert(t() == 1);
