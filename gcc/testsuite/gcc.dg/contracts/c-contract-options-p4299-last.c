/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where the last of -fcontracts-p4299 and -fno-contracts-p4299 decides.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fno-contracts-p4299 -fcontracts-p3100 -fcontracts-p4299" } */

#ifndef __gcc_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f (int x) _Pre (x > 0) { return x; }
