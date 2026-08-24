/* P3595: config file sets pre=observe, assert=ignore.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-configuration-file=${srcdir}/gcc.dg/contracts/contracts-config-kind.json" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;
static int last_kind = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  handler_called++;
  last_kind = stdc_contract_violation_kind(cv);
}

int guarded(int x) _Pre(x > 0)
{
  _ContractAssert(x > 0);
  return x;
}

int main(void)
{
  guarded(-1);

  /* Pre is observe: handler should have been called once for precondition.
     Assert is ignore: handler should NOT have been called for the assert.  */
  if (handler_called != 1)
    __builtin_abort();
  if (last_kind != STDC_CONTRACT_PRE)
    __builtin_abort();

  return 0;
}
