// P4405 premises decided by this TU (M5-GCC): a callee inlined into the
// shadow but not into the real function (provoked as in
// p4405-premise-shadow-only.C), whose precondition this TU ignores, and a
// proof relying on what its code does not do: it calls only const functions,
// so the shadow sees that g is still 1 after the call.  For the inline
// callee another TU's definition, which the real function may call, may
// evaluate the precondition with observe, whose violation handler may store
// to g: the proof is conditional, and nothing is assumed (the real function
// keeps its test of t).  The static callee's copy is this TU's: sound, and
// assumed (the test folded).  p4405-premise-multi-tu-effects.C runs it.
// -fdump-tree-contract_shadow shows each result ("contract obligation N
// (line L): sound").
//
// Mirror: clang/test/Contracts/p4405-premise-shadow-only-effects.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-shadow-only-effects.json" }

[[gnu::const]] int sinkc (int);
int big (int);
int g;

namespace sem_ignore_inline {
inline int h_inline (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sinkc (i * x);
  return s + sinkc (x) + sinkc (x + 1);
}
}

int g_inline (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  g = 1;
  int r = sem_ignore_inline::h_inline (y);
  int t = g;
  contract_assert (t == 1);	// conditional
  return r + (t == 1 ? 10 : 20);
}

namespace sem_ignore_static {
static int h_static (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sinkc (i * x);
  return s + sinkc (x) + sinkc (x + 1);
}
}

int g_static (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  g = 1;
  int r = sem_ignore_static::h_static (y);
  int t = g;
  contract_assert (t == 1);	// sound
  return r + (t == 1 ? 10 : 20);
}

// { dg-final { scan-tree-dump {\(line 47\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 72\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {= sem_ignore_inline::h_inline \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump {= sem_ignore_static::h_static \(y_[0-9]+\(D\)\)} "optimized" } }
// Assumed only for the static callee: `t == 1 ? 10 : 20' folded
// there, kept in g_inline.
// { dg-final { scan-tree-dump-times {contract obligation [0-9]+: assumed} 1 "contract_proven" } }
// { dg-final { scan-tree-dump-times {_[0-9]+ == 1} 1 "optimized" } }
