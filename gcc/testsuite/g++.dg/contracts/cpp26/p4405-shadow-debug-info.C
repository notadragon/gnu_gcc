// D4405 proof shadows and -g (GCC-997): the analysis-only functions --
// proof shadows, override shadows, and the level copies a shadow calls in
// place of a function at another optimization level -- leave the debug
// information as it is without them.  Versioning a function, or inlining
// it, makes it an abstract instance with a concrete out-of-line instance
// (DW_AT_abstract_origin); a shadow is never described, so neither the
// function it copies (f, f2, D::v) nor a function inlined only into it
// (g2, whose call in a predicate the real f2 does not make) nor one the
// real code inlines too (sq) may change.  The object code, the debug
// sections included, is the same with the shadows, without them, at proof
// level 0 and with every evaluation ignore, at every level: -O0 and -O1/-Og
// (shadows at proof level 2, level copies of the callees) and -O2/-O3.
// Nor is an external vtable a shadow loads described (V, whose key
// function is elsewhere: use's predicate calls the inline V::ok by its
// qualified name).  The premise shadows and copies:
// p4405-shadow-debug-info-premises.C.
//
// Mirror: clang/test/Contracts/p4405-shadow-debug-info.cpp and (V)
// p4405-shadow-debug-info-types.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -g -gno-record-gcc-switches" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

int g2 (int i) { return i + 1; }
static inline int sq (int i) { return i * i; }

int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);
  return i;
}

int f2 (int i, int n)
{
  contract_assert (g2 (i) > sq (n));
  return sq (i);
}

struct B { virtual int v (int x) pre (x > 0) post (r : r > 0); };
struct D : B
{
  int v (const int x) override pre (x >= 0) post (r : r > x) { return x + 1; }
};
B *make () { return new D; }

struct V { virtual int f (int); virtual bool ok (int n) const { return n > 0; } };
V *makev () { return new V; }
int use (V *v, int n) { if (n > 1) contract_assert (v->V::ok (n)); return v->f (n); }

// { dg-final { contracts-same-object-code "-O0 -fcontract-evaluation-semantic=ignore" "-O0" "-O0 -fno-contracts-prove-shadow" "-O0 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O1 -fcontract-evaluation-semantic=ignore" "-O1" "-O1 -fno-contracts-prove-shadow" "-O1 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-Og -fcontract-evaluation-semantic=ignore" "-Og" "-Og -fno-contracts-prove-shadow" "-Og -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O2 -fcontract-evaluation-semantic=ignore" "-O2" "-O2 -fno-contracts-prove-shadow" "-O2 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O3 -fcontract-evaluation-semantic=ignore" "-O3" "-O3 -fno-contracts-prove-shadow" "-O3 -fcontracts-prove-optimize=0" } }
