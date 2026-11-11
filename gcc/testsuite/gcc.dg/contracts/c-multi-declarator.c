/* A prototype with a contract specifier followed, in the same declaration,
   by a variable declarator is accepted: the non-function check for the
   variable counts only its own specifiers.

   Mirror: clang/test/Contracts/c-multi-declarator.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

int g (int a) _Pre (a > 0), x;
int g (int a) { return a; }
