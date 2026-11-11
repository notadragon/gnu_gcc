/* DECISIONS.md E179 (1) (GCC-970): on a C input -fcontracts-p3850 implies
   the C per-paper options except one explicitly disabled, whatever the
   order, and turns C contracts on itself even with all three disabled:
   -fsanitize-noncontract-callbacks and a P4405 strategy then warn that
   they have no effect, and C contracts stay on.

   Mirror: clang/test/Contracts/C/c-contract-options-p3850-driver.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fno-contracts-p3100 -fno-contracts-p4298 -fcontracts-p3850 -fno-contracts-p4405 -fsanitize-noncontract-callbacks -fcontracts-prove-fold" } */
/* { dg-warning ".-fsanitize-noncontract-callbacks. has no effect without .-fcontracts-p3100." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-prove-fold. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 } */

#ifndef __gcc_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f (int x) _Pre (x > 0) { return x; }
