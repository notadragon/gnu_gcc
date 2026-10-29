// P3100: with the known-bound pointer-arithmetic check configured, arithmetic
// within [0, N] is still a constant expression; arithmetic past N, which the
// evaluator itself accepts, is undefined, and the check reports it as such
// rather than as a call to the violation entry point.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

constexpr bool f (int i) { int a[3] {}; int *p = a + i; return p != nullptr; }	// { dg-bogus "non-.constexpr. function" }
// { dg-error "undefined behavior is not a constant expression" "" { target *-*-* } .-1 }
static_assert (f (3));
constexpr bool b = f (4);	// { dg-message "in .constexpr. expansion" }
