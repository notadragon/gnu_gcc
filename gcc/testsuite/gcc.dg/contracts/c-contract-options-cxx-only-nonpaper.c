/* -fcontracts and the C++-only non-paper contracts options on a C input
   each say they are valid for C++ but not for C, and do not turn C
   contracts on.  The negative forms are
   c-contract-options-cxx-only-nonpaper-negative.c; beside a C per-paper
   option, c-contract-options-cxx-only-nonpaper-mixed.c.

   Mirror: clang/test/Contracts/C/c-contract-options-cxx-only.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts -fcontract-bypass-rethrowing-local-handler -fcontract-analysis-skip-dead-calls" } */
/* { dg-warning ".-fcontracts. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontract-bypass-rethrowing-local-handler. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontract-analysis-skip-dead-calls. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */

#ifdef __gcc_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
