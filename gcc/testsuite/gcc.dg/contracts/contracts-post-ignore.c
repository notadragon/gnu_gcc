/* Test _Post with ignore semantic: the postcondition is not evaluated (its
   predicate has a side effect that would show) and the handler is not
   called.

   Mirror: clang/test/Contracts/C/contracts-post-ignore.c in the
   llvm_llvm-project fork; owed to EDG, recorded in its
   open-issues/README.md.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=ignore" } */

#include <contracts.h>

static int evaluated;

void handle_contract_violation (const contract_violation_t *cv)
{
  (void) cv;
  __builtin_abort ();
}

static int note (int r) { ++evaluated; return r > 0; }

int positive (int x) _Post (r: note (r))
{
  return x;
}

int main (void)
{
  positive (-1);
  if (evaluated != 0)
    __builtin_abort ();
  return 0;
}
