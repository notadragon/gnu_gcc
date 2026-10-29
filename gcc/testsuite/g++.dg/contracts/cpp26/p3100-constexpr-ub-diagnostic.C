// Undefined behaviour reached during constant evaluation while the P3100
// implicit checks have a checking semantic is diagnosed as the evaluator
// diagnoses it without the checks -- not as a call to the violation entry
// point.  The out-of-range enum conversion, which the evaluator does not
// diagnose by itself, is reported as undefined behaviour.
//
// Mirror: clang/test/Contracts/p3100-constexpr-ub-diagnostic.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-constexpr-ub-diagnostic.json" }

constexpr int d (int x, int y) { return x / y; }	// { dg-bogus "non-.constexpr. function" }
// { dg-error "is not a constant expression" "" { target *-*-* } .-1 }
constexpr int s (int x, int y) { return x << y; }	// { dg-bogus "non-.constexpr. function" }
// { dg-error "right operand of shift expression" "" { target *-*-* } .-1 }
constexpr int fc (double v) { return (int) v; }		// { dg-bogus "non-.constexpr. function" }
// { dg-error "is not a constant expression" "" { target *-*-* } .-1 }
constexpr int g (int x) { [[assume (x > 0)]]; return x; } // { dg-bogus "non-.constexpr. function" }
// { dg-error "failed .assume. attribute assumption" "" { target *-*-* } .-1 }
enum E { e0, e1, e2, e3 };
constexpr E en (int i) { return (E) i; }		// { dg-bogus "non-.constexpr. function" }
// { dg-error "undefined behavior is not a constant expression" "" { target *-*-* } .-1 }

constexpr int v1 = d (1, 0);	// { dg-message "in .constexpr. expansion" }
constexpr int v2 = s (1, 99);	// { dg-message "in .constexpr. expansion" }
constexpr int v3 = fc (1e30);	// { dg-message "in .constexpr. expansion" }
constexpr int v4 = g (-1);	// { dg-message "in .constexpr. expansion" }
constexpr E v5 = en (9);	// { dg-message "in .constexpr. expansion" }
