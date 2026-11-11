// P4405: a compute_semantic facet may return prove or diagnose.  Allowed,
// the evaluation is diagnosed accordingly; outside the label's allowed set
// it is an error, as for any other semantic, and so is a value that is no
// semantic at all (10, and unspecified).
// D4405: a precondition's analysis evaluation is at each call, so each
// function with one is called once, in ae2_calls, and diagnosed there.
//
// Mirror: clang/test/Contracts/p4405-compute-semantic.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

template<evaluation_semantic R>
struct to {
  using assertion_control_object = to;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return R; }
};

template<evaluation_semantic R>
struct to_excluded {
  using assertion_control_object = to_excluded;
  static constexpr evaluation_semantic_set allowed_semantics
    { evaluation_semantic::enforce, evaluation_semantic::observe };
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return R; }
};

constexpr to<evaluation_semantic::prove> to_prove{};
constexpr to<evaluation_semantic::diagnose> to_diagnose{};
constexpr to_excluded<evaluation_semantic::prove> to_prove_excluded{};
constexpr to_excluded<evaluation_semantic::diagnose> to_diagnose_excluded{};
constexpr to<(evaluation_semantic) 10> to_unknown{};
constexpr to<(evaluation_semantic) 0> to_unspecified{};

int a (int x) pre<to_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int b (int x) pre<to_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int c (int x) pre<to_prove_excluded> (x > 0) { return x; }	// { dg-error "not in the allowed evaluation semantics" }
int d (int x) pre<to_diagnose_excluded> (x > 0) { return x; }	// { dg-error "not in the allowed evaluation semantics" }
int e (int x) pre<to_unknown> (x > 0) { return x; }	// { dg-error "not in the allowed evaluation semantics" }
int f (int x) pre<to_unspecified> (x > 0) { return x; }	// { dg-error "not in the allowed evaluation semantics" }

void
ae2_calls (int y)
{
  a (y);	// { dg-error "contract assertion cannot be proven" }
  b (y);	// { dg-warning "contract assertion cannot be proven" }
}
