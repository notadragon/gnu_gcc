struct X { constexpr ~X() {} };
constexpr bool f() {
  X x;
  x.~X();
  return true;        // x is destroyed again at scope exit: undefined
}
static_assert(f());
