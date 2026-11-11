// P4405 stage 3, GCC-907: IPA-SRA does not count the calls a proof shadow
// makes.  The real code ignores g's result, which f's assertion, evaluated only
// in f's shadow, uses; the real calls go to a clone returning nothing, as
// without the shadow, while the shadow keeps calling g.  The object code is
// exactly what it is without the shadows and with every evaluation ignore (see
// p4405-shadow-same-code-called-once.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

extern int sink (int);

struct S { int a, b, c, d; };

__attribute__ ((noinline)) static int g (S *p)
{
  sink (p->a);
  sink (p->b);
  return p->c;
}

int f (int n)
{
  contract_assert (g (nullptr) >= 0);
  S s = { n, n + 1, n + 2, n + 3 };
  g (&s);
  return n;
}

int h (int n)
{
  S s = { n, 1, 2, 3 };
  g (&s);
  return 0;
}

// { dg-final { scan-assembler "_ZL1gP1S\\.isra\\.0" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore" "" "-fno-contracts-prove-shadow" } }
