/* P3290 C trigger API: stdc_handle_enforced_contract_violation invokes the
   handler and then terminates the program (D4299, priority goal #2).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */
/* { dg-shouldfail "enforced trigger terminates" } */

#include <contracts.h>

void handle_contract_violation(const contract_violation_t* cv)
{
  (void) cv;
  /* Handler runs, then the enforced trigger terminates.  */
}

int main(void)
{
  stdc_handle_enforced_contract_violation("manual check failed");
  /* Must not return.  */
  return 0;
}
