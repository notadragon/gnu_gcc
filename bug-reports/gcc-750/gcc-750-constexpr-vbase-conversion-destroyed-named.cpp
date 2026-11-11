#include <memory>
struct V { int v = 1; };
struct D : virtual V { constexpr ~D() {} };
constexpr bool f() {
  D d;
  d.~D();
  V& r = d;            // UB [basic.life]/8: d is outside its lifetime
  (void)r;
  std::construct_at(&d);
  return true;
}
static_assert(f());    // accepted; should be rejected
