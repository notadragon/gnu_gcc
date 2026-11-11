/* A top-level qualifier on the handler's parameter does not change the
   function's type, so a handle_contract_violation declared with a
   "const contract_violation_t *const" parameter is the replacement handler
   and is called (P4299).

   Mirror: clang/test/Contracts/C/contracts-handler-sig-toplevel-const.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */

#include <contracts.h>

static int called;

void handle_contract_violation (const contract_violation_t *const cv)
{
  (void) cv;
  ++called;
}

int guarded (int x) _Pre (x > 0)
{
  return x;
}

int main (void)
{
  guarded (-1);
  if (called != 1)
    __builtin_abort ();
  return 0;
}
