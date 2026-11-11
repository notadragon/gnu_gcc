/* DECISIONS.md E179 (1) (GCC-970): -fcontracts-p3850 on a C input is an
   error with a final -fno-contracts-p4299 given before it too
   (c-contract-options-p3850-p4299-off-after.c).

   Mirror: clang/test/Contracts/C/c-contract-options-p3850-p4299-off.c in
   the llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fno-contracts-p4299 -fcontracts-p3850" } */
/* { dg-error ".-fcontracts-p3850. cannot be combined with .-fno-contracts-p4299." "" { target *-*-* } 0 } */

int i;
