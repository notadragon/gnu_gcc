// P4405: assertion-control labels and the prove and diagnose semantics.  A
// label's allowed_semantics may contain or exclude them; a prove the label
// does not allow becomes diagnose, failing that ignore, and failing both
// the walk goes on to the safer levels.  (Other semantics reaching prove
// and diagnose: p4405-labels-reach.C and p4405-labels-always.C.)
// D4405: a precondition's analysis evaluation is at each call, so each
// function with one is called once, in ae2_calls, and diagnosed there.
//
// Mirror: clang/test/Contracts/p4405-labels.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=prove" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

constexpr allow<prove, diagnose, ignore> with_both{};
constexpr allow<diagnose, ignore, enforce> no_prove{};
constexpr allow<ignore, enforce> neither{};
constexpr allow<enforce> enforce_only{};
constexpr allow<prove> prove_only{};

int a (int x) pre<with_both> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int b (int x) pre<no_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int c (int x) pre<neither> (x > 0) { return x; }	// { dg-bogus "cannot be proven" }
int d (int x) pre<enforce_only> (x > 0) { return x; }	// { dg-bogus "cannot be proven" }
int e (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

// The labels' sets themselves.
static_assert (decltype (with_both)::allowed_semantics.contains (prove));
static_assert (decltype (with_both)::allowed_semantics.contains (diagnose));
static_assert (!decltype (neither)::allowed_semantics.contains (prove));
static_assert (!decltype (neither)::allowed_semantics.contains (diagnose));

void
ae2_calls (int y)
{
  a (y);	// { dg-error "contract assertion cannot be proven" }
  b (y);	// { dg-warning "contract assertion cannot be proven" }
  c (y);	// { dg-bogus "cannot be proven" }
  d (y);	// { dg-bogus "cannot be proven" }
  e (y);	// { dg-error "contract assertion cannot be proven" }
}
