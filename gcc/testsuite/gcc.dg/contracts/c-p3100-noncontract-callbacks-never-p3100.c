/* P3100 (DECISIONS.md E178 (3)): in C as in C++,
   -fsanitize-noncontract-callbacks only means anything when sanitizer
   checks are routed to the contract-violation handler.  With
   -fcontracts-p3100 never given it warns that it has no effect
   (c-p3100-noncontract-callbacks-no-p3100.c turns it off with
   -fno-contracts-p3100).  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p4299 -fsanitize-noncontract-callbacks" } */
/* { dg-warning "'-fsanitize-noncontract-callbacks' has no effect without '-fcontracts-p3100'" "" { target *-*-* } 0 } */

int f (int *p) { return *p; }
