/* DECISIONS.md E175-E178: the contracts option rule in C, where the
   negative form of a contracts option asks for nothing, so it does not
   warn without C contracts, whether given before or after an explicit
   -fno-contracts-p4299 (E178 (5)).  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fno-contracts-allow-assume -fcontracts-p4299 -fno-contracts-p4299 -fcontracts-allow-assume -fno-contracts-allow-assume" } */
/* { dg-bogus "has no effect" "" { target *-*-* } 0 } */

#ifdef __gcc_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
