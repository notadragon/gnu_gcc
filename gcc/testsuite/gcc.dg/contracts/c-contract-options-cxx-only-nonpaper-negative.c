/* The negative forms of -fcontracts and the C++-only non-paper contracts
   options on a C input say they are valid for C++ but not for C too
   (c-contract-options-cxx-only-nonpaper.c).

   Mirror: clang/test/Contracts/C/c-contract-options-cxx-only.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fno-contracts -fno-contract-bypass-rethrowing-local-handler -fno-contract-analysis-skip-dead-calls" } */
/* { dg-warning ".-fno-contracts. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fno-contract-bypass-rethrowing-local-handler. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fno-contract-analysis-skip-dead-calls. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */

#ifdef __gcc_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
