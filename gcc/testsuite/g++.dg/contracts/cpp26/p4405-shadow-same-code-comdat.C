// P4405 stage 3, GCC-907: a static function only a comdat function's real code
// calls joins its comdat group, though the proof shadow of the comdat function,
// never emitted, calls it too.  The object code is exactly what it is without
// the shadows and with every evaluation ignore (see
// p4405-shadow-same-code-called-once.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }
// { dg-require-effective-target comdat_group }

extern int sink (int);

__attribute__ ((noinline)) static int g (int x) { return sink (x) + 1; }

inline int f (int n)
{
  contract_assert (n > 0);
  return g (n) * 2;
}

int (*use) (int) = f;

// { dg-final { scan-assembler {_ZL1gi,"axG",.progbits,_Z1fi,comdat} } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore" "" "-fno-contracts-prove-shadow" } }
