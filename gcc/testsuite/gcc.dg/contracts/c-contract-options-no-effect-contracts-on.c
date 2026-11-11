/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where with C contracts on, here turned on by -fcontracts-p4405, the
   contracts options do not warn, nor the P4405 strategies with P4405 on.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p4405 -fcontracts-allow-assume -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore -fcontracts-client-check=all -fcontracts-definition-check=off -fcontracts-prove-fold -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-assume -fcontracts-prove-callee-premises -fcontracts-prove-optimize=0" } */
/* { dg-bogus "has no effect" "" { target *-*-* } 0 } */

int f (int x) _Pre (x > 0) { return x; }
