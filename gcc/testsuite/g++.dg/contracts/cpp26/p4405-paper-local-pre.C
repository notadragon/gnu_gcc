// D4405R0, "Local Proofs": a precondition with the always_prove label must
// be proven at every call.  f(1) is proven by folding, the guarded call by
// the proof shadow (optimized), and the unguarded f(y) cannot be proven.
// The paper's example, with its comments.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// The paper's always_prove label, and always_diagnose, "defined the same
// way".
struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics = {evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};
struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics = {evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

void f(int x) pre<always_prove>(x > 0);	// { dg-message "the predicate of the contract assertion" }

void g(int y)
{
  f(1);         // OK, proven		{ dg-bogus "cannot be proven|always false" }
  f(y);         // Error, cannot be proven	{ dg-error "contract assertion cannot be proven" }
  if (y > 0) {
    f(y);       // OK, proven		{ dg-bogus "cannot be proven" }
  }
}
