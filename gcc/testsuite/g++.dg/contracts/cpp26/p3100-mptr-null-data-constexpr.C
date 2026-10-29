// P3100: with the null pointer-to-data-member check configured, constant
// evaluation is unchanged: the representation test is not a constant
// expression, and applying a non-null pointer stays a constant expression.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S { int a; int m; };
constexpr int get (int S::*p) { S s { 1, 2 }; return s.*p; }
constexpr bool same (int S::*p) { S s {}; return &(s.*p) == &s.m; }
static_assert (get (&S::a) == 1);
static_assert (get (&S::m) == 2);
static_assert (same (&S::m));
