// P3100: with the null pointer-to-member-function check configured, constant
// evaluation is unchanged: a call through a non-null pointer is a constant
// expression, and a call through a null one is still rejected by the
// evaluator, not as a call to the violation entry point.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S {
  constexpr int f () const { return 1; }
  constexpr virtual int g () const { return 2; }
};
constexpr int call (int (S::*p) () const) { S s; return (s.*p) (); }	// { dg-error "does not designate a .constexpr. function" }
static_assert (call (&S::f) == 1);
static_assert (call (&S::g) == 2);
constexpr int bad = call (nullptr);	// { dg-message "in .constexpr. expansion" }
