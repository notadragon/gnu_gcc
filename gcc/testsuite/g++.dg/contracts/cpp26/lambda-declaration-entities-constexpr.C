// Constant evaluation of preconditions and postconditions whose lambdas
// capture the function's parameters, this, *this and the result name
// (implicitly, explicitly, by copy, by reference, nested, generic): the
// predicates that hold are satisfied, including through a recursive call
// that maps each call's own parameters, and the violated ones are diagnosed.
// (EDG-1/2/39 shapes.)
//
// Mirror: clang/test/Contracts/lambda-declaration-entities-constexpr.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=quick_enforce" }

struct S {
  int m;
  constexpr int f(int x) const pre([this] { return m > 0; }()) {
    return m + x;
  }
  constexpr int g(int x) const pre([&] { return m > x; }()) { return m - x; }
  constexpr int h(int x) const pre([*this] { return m > 0; }()) { return m; }
  constexpr int k(int x) const pre([this](auto j) { return m > j; }(x)) {
    return m;
  }
};
constexpr int f(int x) pre([&] { return x > 0; }()) { return x; }
constexpr int g(int x) pre([x] { return x > 0; }()) { return x; }
constexpr int g2(int x) pre([&x] { return x > 0; }()) { return x; }
constexpr int g3(int x) pre([=] { return [&] { return x > 0; }(); }()) {
  return x;
}
constexpr int g4(int x) pre([&](auto k) { return x > k; }(0)) { return x; }
constexpr int g5(const int &x) pre([&] { return x > 0; }()) { return x; }
constexpr int a() post(r: [r] { return r > 0; }()) { return 1; }
constexpr int b() post(r: [&r] { return r > 0; }()) { return 1; }
constexpr int c() post(r: [&](auto k) { return r + k > 0; }(1)) { return 1; }
constexpr int d(const int x) post(r: [=] { return r > x; }()) { return x + 1; }
constexpr int e(int x, int y) pre([&] { return x < y; }()) { return y - x; }
// The parameter, not a copy made before the call: a recursive call maps the
// proxies of the same specifier to its own parameters.
constexpr int fact(int n)
  pre([&] { return n >= 0 && (n == 0 || fact(n - 1) > 0); }()) {
  return n == 0 ? 1 : n * fact(n - 1);
}
constexpr int one = 1;
static_assert(S{1}.f(1) == 2);
static_assert(S{1}.g(0) == 1);
static_assert(S{1}.h(0) == 1);
static_assert(S{2}.k(1) == 2);
static_assert(f(1) == 1);
static_assert(g(1) == 1);
static_assert(g2(1) == 1);
static_assert(g3(1) == 1);
static_assert(g4(1) == 1);
static_assert(g5(one) == 1);
static_assert(a() == 1);
static_assert(b() == 1);
static_assert(c() == 1);
static_assert(d(1) == 2);
static_assert(e(1, 3) == 2);
static_assert(fact(4) == 24);

namespace violated {
struct S {
  int m;
  constexpr int f(int x) const pre([this] { return m > 0; }()) {  // { dg-error "contract predicate is false" }
    return m + x;
  }
  constexpr int k(int x) const pre([this](auto j) { return m > j; }(x)) {  // { dg-error "contract predicate is false" }
    return m;
  }
};
constexpr int f(int x) pre([&] { return x > 0; }()) { return x; }  // { dg-error "contract predicate is false" }
constexpr int g(int x) pre([x] { return x > 0; }()) { return x; }  // { dg-error "contract predicate is false" }
constexpr int g4(int x) pre([&](auto k) { return x > k; }(0)) { return x; }  // { dg-error "contract predicate is false" }
constexpr int a() post(r: [r] { return r > 0; }()) { return 0; }  // { dg-error "contract predicate is false" }
constexpr int b() post(r: [&r] { return r > 0; }()) { return 0; }  // { dg-error "contract predicate is false" }
constexpr int d(const int x) post(r: [=] { return r > x; }()) { return x; }  // { dg-error "contract predicate is false" }
constexpr int e(int x, int y) pre([&] { return x < y; }()) { return y - x; }  // { dg-error "contract predicate is false" }
static_assert(S{0}.f(1) == 1);
static_assert(S{1}.k(1) == 1);
static_assert(f(0) == 0);
static_assert(g(0) == 0);
static_assert(g4(0) == 0);
static_assert(a() == 0);
static_assert(b() == 0);
static_assert(d(1) == 1);
static_assert(e(3, 1) == -2);
}
