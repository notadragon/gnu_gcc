/* DECISIONS.md E179 (3) (GCC-970): a C++-only contracts per-paper option
   on a C input says it is valid for C++ but not for C, and the C per-paper
   option beside it keeps working: -fcontracts-p3100 turns C contracts on
   (c-contract-options-cxx-only.c).

   Mirror: clang/test/Contracts/C/c-contract-options-cxx-only.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p3400 -fcontracts-p3100" } */
/* { dg-warning ".-fcontracts-p3400. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */

#ifndef __gcc_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f (int x) _Pre (x > 0) { return x; }
