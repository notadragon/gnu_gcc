/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where the final values decide: -fcontracts-p3100 given positively and
   then negatively is off, so a final -fno-contracts-p4299 is no error.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p3100 -fno-contracts-p3100 -fno-contracts-p4299" } */

#ifdef __gcc_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
