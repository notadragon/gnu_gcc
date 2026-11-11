// P4405 stage 3, GCC-907: neither a proof shadow's size nor what is inlined
// into it counts against the unit-growth budget of the real code.  The
// parameters make the unit large (above large-unit-insns) and leave room to
// inline h into only some of its callers, five of which hold an unproven
// evaluation; the object code is exactly what it is without the shadows and
// with every evaluation ignore (see p4405-shadow-same-code-called-once.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven --param large-unit-insns=10 --param inline-unit-growth=50" }

extern int sink (int);

static inline int h (int n)
{
  return sink (n) + sink (n + 1) * sink (n + 2) + sink (n + 3);
}

int f1 (int n) { contract_assert (n > 1); return h (n) + 1; }
int f2 (int n) { contract_assert (n > 2); return h (n) + 2; }
int f3 (int n) { contract_assert (n > 3); return h (n) + 3; }
int f4 (int n) { contract_assert (n > 4); return h (n) + 4; }
int f5 (int n) { contract_assert (n > 5); return h (n) + 5; }
int f6 (int n) { return h (n) + 6; }

// Some calls are inlined, not all.
// { dg-final { scan-assembler "_ZL1hi" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore" "" "-fno-contracts-prove-shadow" } }
