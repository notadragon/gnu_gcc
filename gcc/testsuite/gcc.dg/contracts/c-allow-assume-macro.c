/* -fcontracts-allow-assume is honoured in C: the vendor macro
   __gcc_contracts_allow_assume is defined, and the C contract queries admit
   the assume semantic.

   Mirror: clang/test/Contracts/c-allow-assume-macro.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-allow-assume" } */

#include <contracts.h>

#ifndef __gcc_contracts_allow_assume
#error "__gcc_contracts_allow_assume not defined"
#endif

int f (int x) _Pre (x > 0) { return x; }
