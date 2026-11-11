/* DECISIONS.md E179 (1) (GCC-970): -fcontracts-p3850 on a C input turns C
   contracts on, so it is an error with a final -fno-contracts-p4299,
   whatever the order (c-contract-options-p3850-p4299-off-before.c), as the
   other C per-paper options are (c-contract-options-p4299-off-after.c).
   The options it implies are not diagnosed as well.

   Mirror: clang/test/Contracts/C/c-contract-options-p3850-p4299-off.c in
   the llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p3850 -fno-contracts-p4299" } */
/* { dg-error ".-fcontracts-p3850. cannot be combined with .-fno-contracts-p4299." "" { target *-*-* } 0 } */

int i;
