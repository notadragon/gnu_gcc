// D4405 (DECISIONS.md E142): constant evaluation is never analyzed, and a
// prove it reaches is enforce there -- the one evaluation being made must
// satisfy the predicate, so a violation makes the expression not a
// constant -- and a diagnose is observe, a warning.  However it is reached:
// from an entry without the "constexpr" key (here every evaluation is
// otherwise ignore, so nothing is analyzed), or from a label's allowed set
// (prove_only, diagnose_only, under ignore) or its compute_semantic
// (always_prove, under ignore).  The labels have no runtime semantic, so
// their preconditions are analyzed at each call -- but a call in a
// constexpr variable's initializer is manifestly constant-evaluated, and
// has no analysis evaluation (D4405R0: "During constant evaluation there is
// no analysis evaluation").
//
// Mirror: clang/test/Contracts/p4405-constexpr-enforce.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-constexpr-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic S>
struct only {
  using assertion_control_object = only;
  static constexpr evaluation_semantic_set allowed_semantics{S};
};
struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set allowed_semantics{prove};
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return prove; }
};

constexpr int p (int x) pre (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int pa = p (1);
constexpr int pb = p (-1);

namespace d {
constexpr int f (int x) pre (x > 0) { return x; }	// { dg-warning "contract predicate is false in constant expression" }
constexpr int fb = f (-1);
static_assert (fb == -1);
}

namespace lab {
constexpr int g (int x) pre<only<prove>{}> (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int gb = g (-1);	// { dg-bogus "always false|cannot be proven" }
constexpr int h (int x) pre<only<diagnose>{}> (x > 0) { return x; }	// { dg-warning "contract predicate is false in constant expression" }
constexpr int hb = h (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (hb == -1);
constexpr int k (int x) pre<always_prove_t{}> (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int kb = k (-1);	// { dg-bogus "always false|cannot be proven" }
}
