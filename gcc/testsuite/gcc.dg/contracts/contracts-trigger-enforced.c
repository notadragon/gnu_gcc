/* P3290 C trigger API: stdc_handle_enforced_contract_violation invokes the
   handler and then terminates the program (P4299).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299" } */
/* { dg-shouldfail "enforced trigger terminates" } */

#include <contracts.h>
#include <stdio.h>
#include <stdlib.h>

void handle_contract_violation(const contract_violation_t* cv)
{
  (void) cv;
  /* The user's handler runs, then the enforced trigger terminates.  */
  fputs ("user handler ran\n", stderr);
}

int main(void)
{
  stdc_handle_enforced_contract_violation("manual check failed");
  /* Must not return; if it does, exiting normally makes dg-shouldfail
     report the failure.  */
  return 0;
}

/* { dg-output "user handler ran" } */
