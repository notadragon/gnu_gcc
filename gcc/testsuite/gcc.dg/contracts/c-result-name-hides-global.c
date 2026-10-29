/* The _Post result name is scoped to the postcondition predicate, so in the
   body a file-scope variable of the same name is visible, and f returns it.

   Mirror: clang/test/Contracts/c-result-name-hides-global.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */

#include <contracts.h>

static int handled = 0;
void handle_contract_violation (const contract_violation_t *cv) { ++handled; }

int r = 5;

int f (void) _Post (r: r > 0) { return r; }

int main (void)
{
  return f () == 5 && handled == 0 ? 0 : 1;
}
