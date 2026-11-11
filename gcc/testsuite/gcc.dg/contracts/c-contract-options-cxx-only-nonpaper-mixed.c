/* The C++-only non-paper contracts options on a C input say they are
   valid for C++ but not for C, and the C per-paper option beside them
   keeps working: -fcontracts-p4405 turns C contracts on
   (c-contract-options-cxx-only-nonpaper.c).

   Mirror: clang/test/Contracts/C/c-contract-options-cxx-only.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p4405 -fcontract-analysis-skip-dead-calls -fcontract-bypass-rethrowing-local-handler" } */
/* { dg-warning ".-fcontract-analysis-skip-dead-calls. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontract-bypass-rethrowing-local-handler. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */

#ifndef __gcc_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f (int x) _Pre (x > 0) { return x; }
