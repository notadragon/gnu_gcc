/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where negative per-paper options are always allowed, also with
   -fno-contracts-p4299.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fno-contracts-p4299 -fno-contracts-p3100 -fno-contracts-p4298 -fno-contracts-p4405" } */

#ifdef __gcc_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
