// P4405 premises decided by this TU (T1-GCC): each copy of the real
// function.  g's shadow and g itself inline h (the IPA inliner), so g's
// diagnose evaluation relying on h's enforce precondition is sound and
// assumed in g.  But g is small enough to be inlined early into k, whose
// own size then keeps the IPA inliner from inlining h there: k's copy of g
// calls h out of line -- possibly another TU's h, which may ignore its
// precondition -- so the assumption is dropped in that copy
// (check_assumption_copies), and k keeps its test of y.
//
// Mirror: clang/test/Contracts/p4405-premise-copies.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "--param early-inlining-insns=30 --param large-function-insns=90 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=300 --param max-inline-insns-auto=300 -fno-partial-inlining" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-copies.json" }

int sink (int);
int big (int);

inline int h (int x) pre (x > 0)
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x) + sink (i + x) + sink (i - x);
  for (int i = 0; i < x; i += 2)
    s += sink (i * 3) + sink (i * 5);
  return s + sink (x) + sink (x + 1) + sink (x + 2);
}

int g (int y)
{
  int r = h (y);
  contract_assert (y > 0);	// sound
  return r + (y > 0 ? 1 : 0);
}

int k (int a, int y)
{
  if (a)
    {
      big (1); big (2); big (3); big (4); big (5); big (6); big (7);
      big (8); big (9); big (10); big (11); big (12); big (13); big (14);
      big (15); big (16); big (17); big (18); big (19); big (20);
    }
  return g (y);
}

// { dg-final { scan-tree-dump {\(line 34\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump-times {contract obligation [0-9]+: assumed} 1 "contract_proven" } }
// { dg-final { scan-tree-dump-times {contract obligation [0-9]+: not assumed in this copy} 1 "contract_proven" } }
// Only k still calls h and tests y.
// { dg-final { scan-tree-dump-times {= h \(y_[0-9]+\(D\)\)} 1 "optimized" } }
// { dg-final { scan-tree-dump-times {y_[0-9]+\(D\) > 0} 1 "optimized" } }
