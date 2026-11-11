// P3100: with the past-the-end indirection check configured, an indirection
// through known-bound pointer arithmetic within the array is still a
// constant expression, and one at the bound is diagnosed as undefined: the
// address of the indirection `&*(a + N)', a reference bound to `*(a + N)'
// (both accepted when the check is not configured: GCC-785, GCC-763) and a
// read through nested arithmetic `*(a + 1 + 2)'.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

constexpr bool f (int i) { int a[3] {}; int *p = &*(a + i); return p != nullptr; }	// { dg-error "evaluation of an operation with undefined behavior" }
static_assert (f (2));
constexpr bool b1 = f (3);	// { dg-message "in .constexpr. expansion" }
constexpr int g (int i) { int a[3] {1, 2, 3}; return *(a + 1 + i); }	// { dg-error "evaluation of an operation with undefined behavior" }
static_assert (g (1) == 3);
constexpr int b2 = g (2);	// { dg-message "in .constexpr. expansion" }
constexpr int h (int i) { int a[3] {1, 2, 3}; int &r = *(a + i); return &r - a; }	// { dg-error "evaluation of an operation with undefined behavior" }
static_assert (h (2) == 2);
constexpr int b3 = h (3);	// { dg-message "in .constexpr. expansion" }
