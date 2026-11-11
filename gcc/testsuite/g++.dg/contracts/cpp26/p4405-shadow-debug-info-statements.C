// D4405 proof shadows and -g (GCC-997): statement shapes of
// p4405-prove-optimize-same-code-O0.C -- a region all a statement holds:
// the arm of an `if', of a short-circuit condition, of a nested `if'; the
// body of a `while' loop; a precondition and a postcondition; a predicate
// loading a `bool' -- leave the debug information as it
// is without the shadows: the regions take no discriminators from the
// real code (assign_discriminators).  The object code, the debug sections
// included, is the same with the shadows, without them and at proof level
// 0 (where no region is placed), at -O0, -Og, -O1, -O2 and -O3.  The
// shapes that still differ: open-bug-p4405-shadow-debug-info-residue.C.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -g -gno-record-gcc-switches" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

struct S { int x; volatile int v; };
int g (int);

int f1 (int i, int n) { if (i < n) contract_assert (n > 0); return 1; }
int f2 (S *p, int n) { if (p->x < n) contract_assert (n > 0); return 2; }
int f3 (int i, int n) { if (g (i) < n) contract_assert (n > 0); return 3; }
int f4 (S *p, int n) { if (p->v < n) contract_assert (n > 0); return 4; }
int f6 (S *p, int i, int n) { if (i < n && p->x > 0) contract_assert (n > 0); return 6; }
int f7 (int i, int n) { if (i < n) { if (n > 3) contract_assert (i > 0); } return 7; }
int f12 (int i, int n) pre (i < n) post (r : r > 0) { if (i > 0) contract_assert (n > 1); return 12; }
int f14 (int i, int n) { while (i < n) { contract_assert (i < n); ++i; } return i; }
bool f15 (bool b, int *p) { contract_assert (!b || *p > 0); return b; }

// { dg-final { contracts-same-object-code "-O0 -fcontracts-prove-optimize=0" "-O0" "-O0 -fno-contracts-prove-shadow" "-O0 -fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-Og -fcontracts-prove-optimize=0" "-Og" "-Og -fno-contracts-prove-shadow" "-Og -fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O1 -fcontracts-prove-optimize=0" "-O1" "-O1 -fno-contracts-prove-shadow" "-O1 -fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O2 -fcontracts-prove-optimize=0" "-O2" "-O2 -fno-contracts-prove-shadow" "-O2 -fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0" } }
// { dg-final { contracts-same-object-code "-O3 -fcontracts-prove-optimize=0" "-O3" "-O3 -fno-contracts-prove-shadow" "-O3 -fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0" } }
