// P3100: with the overlapping assignment check configured
// (ub:expr.assign.overlap), constant evaluation is unchanged: comparing
// addresses is not a constant expression, so the check is skipped and the
// assignment from an overlapping union member stays a constant expression,
// as it is without the check.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.assign.overlap:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

union U { int i; short s; };
constexpr int un () { U u; u.s = 1; u.i = u.s; return u.i; }
static_assert (un () == 1);
constexpr int ok (int *p, int *q) { *p = *q; return *p; }
constexpr int run () { int a = 1, b = 2; return ok (&a, &b) + ok (&a, &a); }
static_assert (run () == 4);
