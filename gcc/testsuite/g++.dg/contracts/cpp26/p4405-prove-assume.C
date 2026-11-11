// P4405 -fcontracts-prove-assume (implied by -fcontracts-p4405): a prove or
// diagnose evaluation the proof shadow proves soundly is an assumption in
// the real function, as if evaluated with assume.  In sound (), the shadow
// inlines the static ns::h, whose enforced precondition then proves y > 0
// (h is this TU's code, so the proof is sound), while the real function
// calls h out of line (the parameters provoke that, as in
// p4405-premise-shadow-only.C): only the assumption made of the proven
// contract_assert tells the optimizers that y is positive, which turns
// y / 4 into a shift.  After observe the proof is only conditional (the
// handler may return), so nothing is assumed and the division stays.  After
// assume the proof is sound and assumed too, as Clang's sound () shows; but
// in GCC the real code already knows the predicate after assume, which is
// an assumption there, so the shift appears without
// -fcontracts-prove-assume as well (p4405-prove-assume-after-assume.C).
// The configuration selects each earlier evaluation's semantic by its line;
// every other evaluation proves.  p4405-prove-assume-off.C has
// -fno-contracts-prove-assume.
//
// Mirror: clang/test/Contracts/p4405-prove-assume.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fdump-tree-optimized -fdump-tree-contract_proven" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-prove-assume.json" }

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
  contract_assert (x > 0 && x < 1000);	// line 43: assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = ns::h (y);
  contract_assert (y > 0);	// line 51
  return r + sink (y / 4);
}

int conditional (int x)
{
  contract_assert (x > 0);	// line 57: observe
  contract_assert (x > 0);	// line 58
  return sink (x / 4);
}

int assumed (int x)
{
  contract_assert (x > 0);	// line 64: assume
  contract_assert (x > 0);	// line 65
  return sink (x / 4);
}

// The real sound () calls h.
// { dg-final { scan-tree-dump {ns::h[.a-z0-9]* \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump-times " >> 2;" 2 "optimized" } }
// { dg-final { scan-tree-dump-times " / 4;" 1 "optimized" } }
// { dg-final { scan-tree-dump-times "contract obligation \[0-9\]+: assumed" 2 "contract_proven" } }
// { dg-final { scan-tree-dump-times "contract obligation \[0-9\]+: not assumed" 1 "contract_proven" } }
