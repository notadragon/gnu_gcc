// D4405R0, "Virtual Functions", DECISIONS.md E152: the override shadows'
// regions in destructors -- user-provided and implicitly defined, of
// classes with and without virtual bases, with a throwing member
// destructor, with a precondition of their own -- and the try/finally
// that holds the overridden destructor's postcondition's obligation, are
// shed by the real functions before SSA, which keep their own checks.
// The object code is the same as with -fcontract-analysis-semantic=ignore,
// with and without -fcontracts-prove-override and
// -fcontracts-prove-assume, with the runtime evaluation semantic observe,
// and with -fcontract-checks-outlined (contracts-same-object-code,
// lib/contracts-objects.exp).
// Mirror: clang/test/Contracts/p4405-override-dtor-realcode.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -fno-contracts-prove-assume -O2" }
// { dg-additional-options "-fdump-tree-optimized -fdump-ipa-contract_shadow_clone -Wno-contract-unproven" }

int g;
void sink (int);

struct B {
  virtual ~B () noexcept (false) post (g == 1);
};

struct M { ~M () noexcept (false); int v; };

struct D : B {
  ~D () override { sink (1); }
};

struct E : B {
  M m;
};

struct F : B {
  ~F () override pre (g > 0) { sink (2); }
};

struct V : virtual B {
  int i;
};

struct W : V {
  ~W () override { for (int i = 0; i < g; ++i) sink (i); }
};

void kill (B *p) { delete p; }
void use () { D d; E e; F f; V v; W w; }

// { dg-final { scan-ipa-dump-times "Override shadow" 5 "contract_shadow_clone" } }
// { dg-final { scan-tree-dump-not "__contract_override" "optimized" } }
// { dg-final { scan-tree-dump-not "\\.CONTRACT_" "optimized" } }
// { dg-final { scan-tree-dump "__cxa_contract_violation_pre" "optimized" } }
// { dg-final { contracts-same-object-code "-fcontract-analysis-semantic=ignore" "" "-fno-contracts-prove-override" "-fcontracts-prove-assume" "-fcontracts-prove-assume -fno-contracts-prove-override" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore" "-fcontract-evaluation-semantic=observe" "-fcontract-evaluation-semantic=observe -fcontracts-prove-assume" } }
// { dg-final { contracts-same-object-code "-fcontract-checks-outlined -fcontract-analysis-semantic=ignore" "-fcontract-checks-outlined" "-fcontract-checks-outlined -fno-contracts-prove-override" "-fcontract-checks-outlined -fcontracts-prove-assume" } }
