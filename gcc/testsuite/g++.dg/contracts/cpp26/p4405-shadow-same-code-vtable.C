// D4405 proof shadows change nothing in the real code, through an external
// vtable either (GCC-1000).  V's key function is defined elsewhere, so its
// vtable is external; GCC still knows its initializer, and folds a load
// from it to the virtual function it holds only if that function's
// definition is there to refer to (can_refer_decl_in_current_unit_p).
// Nothing in the real code needs V::ok, an inline virtual function; the
// predicate's qualified call does, in use's proof shadow (or its level
// copy, at -O1 and -Og), and a definition there only for analysis-only
// code is not one the real code may refer to.  So where -fdevirtualize is
// off (-O1, -Og, and -fno-devirtualize) the real `v->ok (n)' stays a
// virtual call, as with every evaluation ignore; where it is on, V::ok is
// kept anyway as a possible target of that call, and the real code is
// devirtualized either way.  The object code is the
// same with the shadows, without them and at proof level 0 as with
// ignore, at -O0, -O1, -Og, -O2, -O3 and -Os, with and without
// -fno-devirtualize.
//
// Mirror: clang/test/Contracts/p4405-shadow-same-code-vtable.cpp in the
// llvm_llvm-project fork (CLANG-914).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

struct V { virtual int f (int); virtual bool ok (int n) const { return n > 0; } };

bool use (int n)
{
  V *v = new V;
  if (n > 1)
    contract_assert (v->V::ok (n));
  return v->ok (n);
}

// { dg-final { contracts-same-object-code "-O0 -fcontract-evaluation-semantic=ignore" "-O0" "-O0 -fno-contracts-prove-shadow" "-O0 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O1 -fcontract-evaluation-semantic=ignore" "-O1" "-O1 -fno-contracts-prove-shadow" "-O1 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-Og -fcontract-evaluation-semantic=ignore" "-Og" "-Og -fno-contracts-prove-shadow" "-Og -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O2 -fcontract-evaluation-semantic=ignore" "-O2" "-O2 -fno-contracts-prove-shadow" "-O2 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O3 -fcontract-evaluation-semantic=ignore" "-O3" "-O3 -fno-contracts-prove-shadow" "-O3 -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-Os -fcontract-evaluation-semantic=ignore" "-Os" "-Os -fno-contracts-prove-shadow" "-Os -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O2 -fno-devirtualize -fcontract-evaluation-semantic=ignore" "-O2 -fno-devirtualize" "-O2 -fno-devirtualize -fno-contracts-prove-shadow" } }
// { dg-final { contracts-same-object-code "-O3 -fno-devirtualize -fcontract-evaluation-semantic=ignore" "-O3 -fno-devirtualize" "-O3 -fno-devirtualize -fno-contracts-prove-shadow" } }
// { dg-final { contracts-same-object-code "-Os -fno-devirtualize -fcontract-evaluation-semantic=ignore" "-Os -fno-devirtualize" "-Os -fno-devirtualize -fno-contracts-prove-shadow" } }
