// [expr.prim.lambda.capture]: a local entity implicitly captured only by
// references inside contract assertions makes the program ill-formed, and
// `this` is a local entity.  Every line marked "error" must be diagnosed.
//
// g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=N lambda-contract-only-capture.cpp
//   CASE=1: accepted ('this' captured only for the contract)
//   CASE=2: accepted (the standard's example f7: y captured only by a lambda
//           inside the precondition)
//   CASE=3: error: 't' is not implicitly captured by a contract assertion,
//           then an internal compiler error while instantiating the lambda
//           (shown as "confused by earlier errors" on a release build)
#if CASE == 1
struct S {
  int m;
  void g() {
    auto s1 = [&] pre(m > 0) {};                 // error
    auto s2 = [&] { contract_assert(m > 0); };   // error
  }
};
#elif CASE == 2
void h() {
  bool y = true;
  auto n1 = [=] pre([=]{ return y; }()) {};      // error
}
#elif CASE == 3
void h(int t) {
  auto t1 = [=](auto u) pre(u > t) {};           // error, then ICE
  t1(1);
}
#else
#error define CASE
#endif
