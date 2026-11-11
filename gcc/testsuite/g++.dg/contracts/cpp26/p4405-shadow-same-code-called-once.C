// P4405 stage 3, GCC-907: a proof shadow is not a caller of the real code.
// Without -fcontracts-prove-assume, the object code is exactly what it is
// without the shadows and with every evaluation ignore, here where the
// inliner's view of the callers decides: big is called once by real code, from
// f, which holds an unproven evaluation (its shadow calls big too); big2 is
// called once by the real code of g and once more by g's assertion, which only
// the shadow evaluates; big3, a comdat function, is called once from h.  Each
// is inlined into its real caller and not emitted, as when called once.  The
// dg-final code compiles the file again with ignore and compares the object
// code with and without the shadows to it.
//
// Mirror: clang/test/Contracts/p4405-shadow-real-inlining.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

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
  contract_assert (n > 0);
  return r + n / 4;
}

static int big2 (int n)
{
  int s = 0;
  for (int i = 0; i < n; ++i)
    {
      s += sink (i) * 3;
      s ^= sink (s);
      s += sink (s + i);
    }
  if (s > 100)
    s = sink (s) + sink (s + 1) + sink (s + 2) + sink (s + 3);
  return s;
}

int g (int n)
{
  contract_assert (big2 (n + 1) > 0);
  return big2 (n) + 1;
}

inline int big3 (int n)
{
  int s = 0;
  for (int i = 0; i < n; ++i)
    {
      s += sink (i) * 3;
      s ^= sink (s);
      s += sink (s + i);
    }
  for (int i = 0; i < n; ++i)
    s ^= sink (s - 1);
  if (s > 100)
    s = sink (s) + sink (s + 1) + sink (s + 2) + sink (s + 3);
  return s;
}

int h (int n)
{
  contract_assert (n != 3);
  return big3 (n) - 1;
}

// { dg-final { scan-assembler-not "_ZL3bigi" } }
// { dg-final { scan-assembler-not "_ZL4big2i" } }
// { dg-final { scan-assembler-not "_Z4big3i" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore" "" "-fno-contracts-prove-shadow" } }
