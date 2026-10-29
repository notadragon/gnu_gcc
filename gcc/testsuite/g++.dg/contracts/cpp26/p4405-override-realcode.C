// D4405R0, "Virtual Functions": the override shadows' regions -- and the
// try/finally that an overrider without postconditions of its own gets
// for the overridden function's postconditions' obligations, the
// snapshots of `this', of the parameters and of a covariant result --
// are shed by the real functions before SSA, which keep their own checks:
// F::g's precondition and F::own's postcondition.  The object code is
// the same as with -fcontract-analysis-semantic=ignore, with and without
// -fcontracts-prove-override and -fcontracts-prove-assume, with the
// runtime evaluation semantic observe, and with -fcontract-checks-outlined
// (contracts-same-object-code, lib/contracts-objects.exp).
// Mirror: clang/test/Contracts/p4405-override-realcode.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -fno-contracts-prove-assume -O2" }
// { dg-additional-options "-fdump-tree-optimized -fdump-ipa-contract_shadow_clone -Wno-contract-unproven" }

struct R { R (); R (const R &); ~R (); int v; };
int sink (int);

struct X { int pad; virtual void x (); };

struct B {
  virtual int f (int x) pre (x > 0) post (r: r > 0);
  virtual int g (int x) noexcept pre (x > 0) post (r: r > 0);
  virtual int h (int x) pre (x > 0) post (r: r > 0);
  virtual R make (int x) pre (x > 0) post (r: r.v > 0);
  virtual B *self () post (r: r == this);
  virtual int p (const int x) post (r: r == x);
  virtual int own (const int x) pre (x > 0) post (r: r > 0);
};

struct F : X, B {
  virtual void key ();
  int f (int x) override pre (x >= 1) { return sink (x); }
  int g (int x) noexcept override { return sink (x) + x; }
  int h (int x) override { if (x > 100) throw 1; return sink (x); }
  R make (int x) override { R r; r.v = sink (x); return r; }
  F *self () override { sink (0); return this; }
  int p (int x) override { int r = x; x = sink (x); return r + x; }
  int own (const int x) override post (r: r >= x) { return x + sink (x); }
};

// The key function emits the vtable, and with it the overriders.
void F::key () {}

// { dg-final { scan-ipa-dump-times "Override shadow" 7 "contract_shadow_clone" } }
// { dg-final { scan-tree-dump-not "__contract_override" "optimized" } }
// { dg-final { scan-tree-dump-not "\\.CONTRACT_" "optimized" } }
// { dg-final { scan-tree-dump "__cxa_contract_violation_pre" "optimized" } }
// { dg-final { scan-tree-dump "__cxa_contract_violation_post" "optimized" } }
// { dg-final { contracts-same-object-code "-fcontract-analysis-semantic=ignore" "" "-fno-contracts-prove-override" "-fcontracts-prove-assume" "-fcontracts-prove-assume -fno-contracts-prove-override" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore" "-fcontract-evaluation-semantic=observe" "-fcontract-evaluation-semantic=observe -fcontracts-prove-assume" } }
// { dg-final { contracts-same-object-code "-fcontract-checks-outlined -fcontract-analysis-semantic=ignore" "-fcontract-checks-outlined" "-fcontract-checks-outlined -fno-contracts-prove-override" "-fcontract-checks-outlined -fcontracts-prove-assume" } }
