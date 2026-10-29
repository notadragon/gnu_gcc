// D4405 ("Adjusting to the Allowed Semantics", "Analyzing Assertions With
// Restricted Semantics"): the standard labels when the build configures
// prove for the runtime evaluations, which step 2 moves to the analysis
// evaluation.  symbolic now allows prove, so its assertions are proven and
// still never checked; always_ignore allows neither, so they are never
// analyzed.  terminating allows prove too, but its runtime evaluations are
// still adjusted to enforce (step 5, which never reaches prove), as the
// constant evaluation shows -- configuration never lets a check be skipped
// when the label allows a runtime semantic.  The precondition is analyzed
// with prove at each call (D4405), but not at t (-1), which is manifestly
// constant-evaluated, and so has no analysis evaluation; each function is
// called once in ae2_calls.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=prove" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace L = std::contracts::labels;

int a (int x) pre<L::symbolic> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int b (int x) pre<L::always_ignore> (x > 0) { return x; }	// { dg-bogus "cannot be proven|no valid" }
int c () pre<L::symbolic> (true) { return 0; }		// { dg-bogus "cannot be proven|no valid" }

constexpr int t (int x) pre<L::terminating> (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int good = t (1);
constexpr int bad = t (-1);	// { dg-bogus "always false|cannot be proven" }

void
ae2_calls (int y)
{
  a (y);	// { dg-error "contract assertion cannot be proven" }
  b (y);	// { dg-bogus "cannot be proven|no valid" }
  c ();	// { dg-bogus "cannot be proven|no valid" }
}
