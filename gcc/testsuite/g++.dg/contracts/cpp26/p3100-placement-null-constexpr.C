// P3100: with the null placement-new check configured, a placement new on a
// non-null pointer is still a constant expression, and one on a null
// pointer is diagnosed as undefined, an empty class too (accepted when the
// check is not configured: GCC-779).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.new.non.allocating.null:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <new>
constexpr int good () { int x = 0; int *p = new (&x) int (3); return *p; }
static_assert (good () == 3);
constexpr int bad () { int *p = nullptr; new (p) int (3); return 0; }	// { dg-error "evaluation of an operation with undefined behavior" }
constexpr int b = bad ();	// { dg-message "in .constexpr. expansion" }
struct E {};
constexpr int bad_empty () { E *p = nullptr; new (p) E; return 0; }	// { dg-error "evaluation of an operation with undefined behavior" }
constexpr int be = bad_empty ();	// { dg-message "in .constexpr. expansion" }
