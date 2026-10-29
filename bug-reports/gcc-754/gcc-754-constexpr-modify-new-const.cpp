constexpr int f() {
  const int* p = new const int(3);
  *const_cast<int*>(p) = 4;      // UB [dcl.type.cv]/4
  int r = *p;
  delete p;
  return r;
}
static_assert(f() == 4);         // passes
