/* Test _Post with quick_enforce semantic: traps without calling the
   handler.

   Mirror: clang/test/Contracts/C/contracts-post-quick-enforce.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=quick_enforce" } */
/* { dg-shouldfail "quick_enforce traps" } */

#include <contracts.h>
#include <stdlib.h>

/* quick_enforce never calls the handler: a call is a failure, and
   exiting normally is how a dg-shouldfail test reports one.  */
void handle_contract_violation (const contract_violation_t *cv)
{
  (void) cv;
  _Exit (0);
}

int positive (int x) _Post (r: r > 0)
{
  return x;
}

int main (void)
{
  positive (-1);
  return 0;
}
