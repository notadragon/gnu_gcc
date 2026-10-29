// Watch test for GCC-907: a proof shadow must not change how the real code
// is compiled when nothing is assumed, but GCC's shadow is one more caller
// of every function its original calls, so the inliner can decide
// differently for the real code.  Here the large static big is called once,
// from f; with ignore, or with -fno-contracts-prove-shadow, big is inlined
// into f and disappears.  With the shadow (f holds an unproven diagnose
// evaluation), big has two callers, is called out of line from f and is
// emitted -- with or without -fcontracts-prove-assume.  Callers that are
// shadows should not count in the inliner's decisions for the real code.
//
// Mirror: clang/test/Contracts/p4405-shadow-real-inlining.cpp in the
// llvm_llvm-project fork; Clang passes (its shadows are a separate module).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose" }

extern int sink (int);

static int big (int n)
{
  int s = 0;
  for (int i = 0; i < n; ++i)
    {
      s += sink (i) * 3;
      s ^= sink (s);
      s += sink (s + i);
    }
  for (int i = 0; i < n; ++i)
    {
      s += sink (i + 7) * 5;
      s ^= sink (s - 1);
    }
  if (s > 100)
    s = sink (s) + sink (s + 1) + sink (s + 2) + sink (s + 3);
  return s;
}

int f (int n)
{
  int r = big (n);
  contract_assert (n > 0);	// { dg-warning "contract assertion cannot be proven" }
  return r + n / 4;
}

// { dg-final { scan-assembler-not "_ZL3bigi" { xfail *-*-* } } }
