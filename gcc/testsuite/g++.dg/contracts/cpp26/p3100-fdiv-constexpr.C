// P3100: with the floating-point division-by-zero check configured, constant
// evaluation is unchanged: a division by a nonzero value is a constant
// expression, and a division by zero is still rejected by the evaluator, as
// the evaluator rejects it, not as a call to the violation entry point.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

constexpr double dz (double z) { return 1.0 / z; }	// { dg-bogus "non-.constexpr. function" }
// { dg-error "is not a constant expression" "" { target *-*-* } .-1 }
static_assert (dz (2.0) == 0.5);
constexpr double d = dz (0.0);	// { dg-message "in .constexpr. expansion" }
