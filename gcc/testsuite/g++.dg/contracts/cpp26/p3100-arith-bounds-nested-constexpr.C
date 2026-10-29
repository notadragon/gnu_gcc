// P3100: with the known-bound pointer-arithmetic checks configured, nested
// arithmetic, an indirection through it and a subscript of it within the
// array are still constant expressions; nested arithmetic past the bound,
// which the evaluator itself accepts, is undefined, and the check reports it
// as such, as it does arithmetic written on the array.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:enforce,ub:expr.unary.dereference.invalid.known:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

constexpr int f (int i, int j) {
  int a[4] {1, 2, 3, 4};
  int *p = (a + i) + j;
  int *q = &(a + 1)[j];
  return *(a + i + j - 1) + (a + 1)[i] + (p - q);
}
static_assert (f (1, 2) == 3 + 3 + 0);
static_assert (f (2, 2) == 4 + 4 + 1);

constexpr bool g (int i) { int a[3] {}; int *p = (a + 1) + i; return p != nullptr; }	// { dg-bogus "non-.constexpr. function" }
// { dg-error "undefined behavior is not a constant expression" "" { target *-*-* } .-1 }
static_assert (g (2));
constexpr bool b = g (3);	// { dg-message "in .constexpr. expansion" }
