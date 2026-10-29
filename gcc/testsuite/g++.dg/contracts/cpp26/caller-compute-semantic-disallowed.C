// A label's compute_semantic returns ignore, which its allowed_semantics
// excludes.  Under P3595 caller-side checking (caller observe, callee
// ignore), the caller-side resolution validates the result against the
// allowed set of a caller-side evaluation, as the callee side does against
// its own.  Under D4405 ("Adjusting to the Allowed Semantics") a
// caller-side evaluation may always be ignore -- the callee-side one is
// made with an allowed semantic -- so ignore is accepted here; a result
// outside that set (assume, not allowed without -fcontracts-allow-assume)
// is diagnosed.
//
// Mirror: clang/test/Contracts/caller-compute-semantic-disallowed.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/open-bug-caller-side.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct L {
  using assertion_control_object = L;
  static constexpr evaluation_semantic_set allowed_semantics
    = evaluation_semantic_set { evaluation_semantic::observe,
				evaluation_semantic::enforce };
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::ignore; }
};
constexpr L l{};

struct M {
  using assertion_control_object = M;
  static constexpr evaluation_semantic_set allowed_semantics
    = evaluation_semantic_set { evaluation_semantic::observe,
				evaluation_semantic::enforce };
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::assume; }
};
constexpr M m{};

void f (int x) pre<l> (x > 0);	// { dg-bogus "not in the allowed evaluation semantics" }
void g () { f (1); }

void h (int x) pre<m> (x > 0);	// { dg-error "not in the allowed evaluation semantics" }
void k () { h (1); }
