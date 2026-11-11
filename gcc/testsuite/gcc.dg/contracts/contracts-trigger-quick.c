/* P3290 C trigger API: stdc_handle_quick_enforced_contract_violation
   terminates immediately, without invoking the handler (P4299).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299" } */
/* { dg-shouldfail "quick-enforced trigger terminates" } */

#include <contracts.h>
#include <stdlib.h>

void handle_contract_violation(const contract_violation_t* cv)
{
  /* quick_enforce never calls the handler: a call is a failure, and
     exiting normally is how a dg-shouldfail test reports one.  */
  (void) cv;
  _Exit (0);
}

int main(void)
{
  stdc_handle_quick_enforced_contract_violation("manual check failed");
  return 0;
}
