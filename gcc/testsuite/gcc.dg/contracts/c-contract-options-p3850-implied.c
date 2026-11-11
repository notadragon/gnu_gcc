/* DECISIONS.md E179 (1) (GCC-970): on a C input -fcontracts-p3850 implies
   -fcontracts-p3100 and -fcontracts-p4405, so neither
   -fsanitize-noncontract-callbacks nor a P4405 strategy warns that it has
   no effect.  P4298 is c-contract-options-p3850-implied-p4298.c; an
   explicitly disabled option is c-contract-options-p3850-implied-off.c.

   Mirror: clang/test/Contracts/C/c-contract-options-p3850-driver.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p3850 -fsanitize-noncontract-callbacks -fcontracts-prove-fold" } */
/* { dg-bogus "has no effect" "" { target *-*-* } 0 } */

int f (int x) _Pre (x > 0) { return x; }
