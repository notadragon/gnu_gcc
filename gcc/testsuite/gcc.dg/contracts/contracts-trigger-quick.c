/* P3290 C trigger API: stdc_handle_quick_enforced_contract_violation
   terminates immediately, without invoking the handler (D4299).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */
/* { dg-shouldfail "quick-enforced trigger terminates" } */

#include <contracts.h>

void handle_contract_violation(const contract_violation_t* cv)
{
  (void) cv;
  /* The quick-enforced trigger must not call the handler; if it did and
     the handler returned, the program would exit 0 and the test would
     (correctly) be reported as an unexpected pass.  */
  __builtin_exit(0);
}

int main(void)
{
  stdc_handle_quick_enforced_contract_violation("manual check failed");
  return 0;
}
