// D4405, step 6 of "Adjusting to the Allowed Semantics": the adjusted
// runtime semantic is passed to compute_semantic.  A prove or diagnose
// result moves to the analysis evaluation, which takes the stronger and must
// allow it, and the runtime evaluation becomes ignore, which must be in the
// runtime allowed set -- R, or {ignore} only when R is empty, so a
// compute_semantic result never lets a callee-side check be skipped that
// the label requires.  Any other result must be in the runtime allowed set.
// Otherwise the program is ill-formed.
// D4405: a precondition's analysis evaluation is at each call, so each
// function with one is called once, in ae2_calls, and diagnosed there.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic R, evaluation_semantic... S>
struct to {
  using assertion_control_object = to;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return R; }
};

template<evaluation_semantic R, evaluation_semantic A>
struct to_analyzed {
  using assertion_control_object = to_analyzed;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return R; }
  constexpr evaluation_semantic analysis_semantic () const { return A; }
};

// R is empty: the callee-side runtime evaluation may be ignore.
constexpr to<ignore, prove> a_l{};
// R is {enforce}: ignore is not allowed, from the result or from a moved
// prove or diagnose.
constexpr to<ignore, enforce, prove> b_l{};
constexpr to<diagnose, enforce, diagnose> c_l{};
// N is empty: diagnose is not allowed for the analysis evaluation.
constexpr to<diagnose, enforce, quick_enforce> d_l{};
// R holds ignore.
constexpr to<diagnose, ignore, diagnose> e_l{};
constexpr to<observe, ignore, observe, diagnose> f_l{};
// The stronger of the result and the facet.
constexpr to_analyzed<diagnose, prove> g_l{};
constexpr to_analyzed<prove, diagnose> h_l{};

int a (int x) pre<a_l> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int b (int x) pre<b_l> (x > 0) { return x; }	// { dg-error "not in the allowed evaluation semantics" }
int c (int x) pre<c_l> (x > 0) { return x; }	// { dg-error "not in the allowed evaluation semantics" }
int d (int x) pre<d_l> (x > 0) { return x; }	// { dg-error "not in the allowed evaluation semantics" }
int e (int x) pre<e_l> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int f (int x) pre<f_l> (x > 0) { return x; }	// { dg-bogus "cannot be proven|not in the allowed" }
int g (int x) pre<g_l> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int h (int x) pre<h_l> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  a (y);	// { dg-error "contract assertion cannot be proven" }
  e (y);	// { dg-warning "contract assertion cannot be proven" }
  f (y);	// { dg-bogus "cannot be proven|not in the allowed" }
  g (y);	// { dg-error "contract assertion cannot be proven" }
  h (y);	// { dg-error "contract assertion cannot be proven" }
}
