// P4405 stage 3, GCC-907: identical code folding merges nothing with code only
// a proof shadow needs.  check, which only f's assertion calls (so only f's
// shadow), has the body of r; r is emitted as itself, not as an alias of check,
// as without the shadow.  The object code is exactly what it is without the
// shadows and with every evaluation ignore (see
// p4405-shadow-same-code-called-once.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

extern int sink (int);

__attribute__ ((noinline)) static int check (int x)
{
  return sink (x) + sink (x * 2) + sink (x * 3);
}

__attribute__ ((noinline)) int r (int x)
{
  return sink (x) + sink (x * 2) + sink (x * 3);
}

int f (int n)
{
  contract_assert (check (n) > 0);
  return r (n);
}

// { dg-final { scan-assembler-not "_ZL5checki" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore" "" "-fno-contracts-prove-shadow" } }
