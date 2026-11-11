/* In a void function, "return g ();" (a GNU extension) runs g () before the
   postcondition is checked, so a postcondition that g () establishes holds.

   Mirror: clang/test/Contracts/c-return-void-expr.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -Wno-pedantic" } */

#include <contracts.h>

static int handled = 0;
void handle_contract_violation (const contract_violation_t *cv) { ++handled; }

static int v;
static void g (void) { v = 1; }

void f (void) _Post (v == 1) { return g (); }

int main (void)
{
  f ();
  return v == 1 && handled == 0 ? 0 : 1;
}
