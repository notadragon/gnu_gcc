constexpr bool f() {
  int* ip = new int(1);
  int& i = *ip;
  delete ip;
  int& j = i;                    // UB [basic.compound]/6, [expr.type]
  (void)j;
  return true;
}
static_assert(f());
