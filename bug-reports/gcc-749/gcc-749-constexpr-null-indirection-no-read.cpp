struct S { static constexpr int sf() { return 1; } static constexpr int k = 1; };
constexpr bool f() {
  int* p = nullptr;
  *p;                       // undefined: indirection through a null pointer
  S* sp = nullptr;
  sp->sf();                 // undefined: *sp is evaluated
  (void) sp->k;             // undefined: *sp is evaluated
  return true;
}
static_assert(f());
using F = int();
constexpr bool faddr(F* fp) { return &*fp == nullptr; }
static_assert(faddr(nullptr));      // undefined: *fp with fp null
constexpr int fdisc(F* fp) { *fp; return 1; }
static_assert(fdisc(nullptr) == 1); // undefined: *fp with fp null
