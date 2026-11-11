// P4405 stage 3, GCC-907: the frequency IPA propagates to a function from its
// callers does not count the calls a proof shadow makes.  The real code calls g
// only from the cold function c; f's assertion, evaluated only in f's shadow,
// calls it too.  g is unlikely executed and placed as such, as without the
// shadow.  The object code is exactly what it is without the shadows and with
// every evaluation ignore (see p4405-shadow-same-code-called-once.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

extern int sink (int);

__attribute__ ((noinline)) static int g (int x)
{
  return sink (x) + sink (x + 1);
}

__attribute__ ((cold)) int c (int n) { return g (n); }

int f (int n)
{
  contract_assert (g (n) > 0);
  return sink (n);
}
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore" "" "-fno-contracts-prove-shadow" } }
