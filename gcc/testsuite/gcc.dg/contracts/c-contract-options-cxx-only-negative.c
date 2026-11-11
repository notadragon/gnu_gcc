/* DECISIONS.md E179 (3) (GCC-970): the negative form of a C++-only
   contracts per-paper option on a C input says it is valid for C++ but not
   for C too (c-contract-options-cxx-only.c).

   Mirror: clang/test/Contracts/C/c-contract-options-cxx-only.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fno-contracts-p3400 -fno-contracts-p4451" } */
/* { dg-warning ".-fno-contracts-p3400. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fno-contracts-p4451. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */

#ifdef __gcc_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
