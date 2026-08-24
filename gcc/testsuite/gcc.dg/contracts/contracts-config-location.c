/* P3595 location-based config selection for C: a rule keyed on the source
   file name selects the observe semantic for contracts in this file
   (D4299).  The default semantic is enforce (terminating); observe returns
   after the handler, so reaching the end of main proves the location rule
   matched.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-configuration-file=${srcdir}/gcc.dg/contracts/contracts-config-location.json" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  handler_called++;
  if (stdc_contract_violation_semantic(cv) != STDC_CONTRACT_OBSERVE)
    __builtin_abort();
}

int guarded(int x) _Pre(x > 0)
{
  return x;
}

int main(void)
{
  guarded(-1);
  if (handler_called != 1)
    __builtin_abort();
  return 0;
}
