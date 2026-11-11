/* Test _ContractAssert with quick_enforce: traps without calling handler.  */
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

int main(void)
{
  int x = -1;
  _ContractAssert(x > 0);
  return 0;
}
