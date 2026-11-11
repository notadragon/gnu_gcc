// P4405 stage 3, GCC-907: IPA-CP does not count the calls a proof shadow makes.
// g is called with the constant 7 by the real code, and with a variable by f's
// assertion, which only f's shadow evaluates; the real calls go to a clone for
// 7, as without the shadow.  The object code is exactly what it is without the
// shadows and with every evaluation ignore (see
// p4405-shadow-same-code-called-once.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

extern int sink (int);

__attribute__ ((noinline)) static int g (int x, int k)
{
  int s = 0;
  for (int i = 0; i < k; ++i)
    s += sink (x + i);
  return s;
}

int f (int n)
{
  contract_assert (g (n, n) >= 0);
  return g (n, 7) + 1;
}

int h (int n) { return g (n + 1, 7); }

// { dg-final { scan-assembler "_ZL1gii\\.constprop\\.0" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore" "" "-fno-contracts-prove-shadow" } }
