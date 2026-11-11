/* P3100 (DECISIONS.md E178 (3)): in C as in C++,
   -fsanitize-noncontract-callbacks only means anything when sanitizer
   checks are routed to the contract-violation handler.  Its
   negative form asks for nothing, so it does not warn without
   -fcontracts-p3100, also after the positive form.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fsanitize-noncontract-callbacks -fno-sanitize-noncontract-callbacks" } */
/* { dg-bogus "has no effect" "" { target *-*-* } 0 } */

int f (int *p) { return *p; }
