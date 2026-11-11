/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where -fcontracts-p3100 turns -fcontracts-p4299 on as a C++ per-paper
   option turns -fcontracts on.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p3100" } */

#ifndef __gcc_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f (int x) _Pre (x > 0) { return x; }
