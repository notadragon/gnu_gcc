// E6: a P3100 implicit UB check does not participate in constant evaluation.
// The constexpr evaluator still rejects the undefined operation directly with
// its normal diagnostic; the configured contract semantic (observe here) does
// not turn the UB into a recoverable contract violation.  (P3100 instrumentation
// is a runtime facility; constant expressions already forbid the UB it guards.)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontract-evaluation-semantic=observe" }

constexpr int kMin = -__INT_MAX__ - 1;

// Division by zero: rejected as non-constant regardless of the observe semantic.
constexpr int divz (int a, int b) { return a / b; } // { dg-error "not a constant expression" }
// Signed overflow INT_MIN / -1: rejected as overflow in a constant expression.
constexpr int ovf (int a, int b) { return a / b; } // { dg-error "overflow in constant expression" }

constexpr int z = divz (1, 0);
constexpr int o = ovf (kMin, -1);
