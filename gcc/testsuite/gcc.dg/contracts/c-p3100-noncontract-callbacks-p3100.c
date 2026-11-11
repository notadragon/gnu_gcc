/* P3100 (DECISIONS.md E178 (3)): in C as in C++,
   -fsanitize-noncontract-callbacks only means anything when sanitizer
   checks are routed to the contract-violation handler.  With
   -fcontracts-p3100 on it does not warn, whether given before or after
   it.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fsanitize-noncontract-callbacks -fno-contracts-p3100 -fcontracts-p3100" } */
/* { dg-bogus "has no effect" "" { target *-*-* } 0 } */

int f (int *p) { return *p; }
