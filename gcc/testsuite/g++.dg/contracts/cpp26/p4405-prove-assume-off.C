// P4405: with -fno-contracts-prove-assume a soundly proven evaluation is not
// an assumption: the division p4405-prove-assume.C turns into a shift
// stays, and nothing in the real function remains of the proof.  As there,
// sound ()'s shadow inlines the static ns::h, whose enforced precondition
// proves y > 0 soundly, while the real function calls h out of line (the
// parameters provoke that, as in p4405-premise-shadow-only.C).
//
// Mirror: clang/test/Contracts/p4405-prove-assume-off.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -fno-contracts-prove-assume -O2" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fdump-tree-optimized -fdump-tree-gimple" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-prove-assume-off.json" }

int sink (int);
int big (int);

namespace ns {
static int h (int x) pre (x > 0)	// enforce
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int sound (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// line 32: assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = ns::h (y);
  contract_assert (y > 0);	// line 40
  return r + sink (y / 4);
}

int conditional (int x)
{
  contract_assert (x > 0);	// line 46: observe
  contract_assert (x > 0);	// line 47
  return sink (x / 4);
}

// The real sound () calls h.
// { dg-final { scan-tree-dump {ns::h[.a-z0-9]* \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump-times " / 4;" 2 "optimized" } }
// { dg-final { scan-tree-dump-not "CONTRACT_PROVEN|ASSUME" "gimple" } }
