#include <memory>
struct S { int i = 0; constexpr int g() const { return 7; } constexpr ~S() {} };
constexpr int f() {
  S s;
  s.~S();
  int r = s.g();       // UB [basic.life]/7: member call outside lifetime
  std::construct_at(&s);
  return r;
}
static_assert(f() == 7);   // accepted; clang: "member call on object outside its lifetime"
