/* The _Post result name is scoped to the postcondition predicate (P4299):
   the body cannot name it.  See c-result-name-hides-global.c for the
   valid-code shape.

   Mirror: clang/test/Contracts/c-result-name-in-body.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

int f (void) _Post (r: r > 0)
{
  return r;  /* { dg-error ".r. undeclared" } */
}
