/* P3290 C trigger API: stdc_handle_observed_contract_violation invokes the
   handler with kind == MANUAL and the observe semantic, then returns
   normally (D4299, priority goal #2).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;
static int last_kind = 0;
static int last_semantic = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  handler_called++;
  last_kind = stdc_contract_violation_kind(cv);
  last_semantic = stdc_contract_violation_semantic(cv);
}

int main(void)
{
  stdc_handle_observed_contract_violation("manual check failed");
  /* The observed variant returns after the handler completes.  */
  if (handler_called != 1)
    __builtin_abort();
  if (last_kind != STDC_CONTRACT_MANUAL)
    __builtin_abort();
  if (last_semantic != STDC_CONTRACT_OBSERVE)
    __builtin_abort();
  return 0;
}
