// D4405R0, "Virtual Functions": an overridden function's postcondition is
// analyzed in an override shadow with its own analysis semantic: here the
// build's is ignore, so one labeled always_prove must be proven in every
// override, and the others are not analyzed (no override shadow for E).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-ipa-contract_shadow_clone" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic S>
struct always {
  using assertion_control_object = always;
  static constexpr evaluation_semantic_set allowed_semantics{S};
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return S; }
};
constexpr always<prove> always_prove{};

struct B {
  virtual int f (int x) post<always_prove> (r: r > 0);	// { dg-message "the predicate of the contract assertion" }
  virtual int g (int x) post (r: r > 0);
};

struct C : B {
  int f (int x) override { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven" }
};

struct D : B {
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
  { return x; }
};

struct E : B {
  int g (int x) override { return x; }	// { dg-bogus "cannot be proven" }
};

void use () { C c; D d; E e; }

// { dg-final { scan-tree-dump {\(line 26, override pair [0-9]+\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 26, override pair [0-9]+\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump-not {\(line 27,} "contract_shadow" } }
// { dg-final { scan-ipa-dump-times "Override shadow" 2 "contract_shadow_clone" } }
