/* P3100 (DECISIONS.md E178 (3)): in C as in C++,
   -fsanitize-noncontract-callbacks only means anything when sanitizer
   checks are routed to the contract-violation handler.  Without
   -fcontracts-p3100 in effect, here turned off by a later
   -fno-contracts-p3100, it warns that it has no effect.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p4299 -fcontracts-p3100 -fsanitize-noncontract-callbacks -fno-contracts-p3100" } */
/* { dg-warning "'-fsanitize-noncontract-callbacks' has no effect without '-fcontracts-p3100'" "" { target *-*-* } 0 } */

int f (int *p) { return *p; }
