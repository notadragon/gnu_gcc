/* Test function with _Pre + _Post + _ContractAssert: all three fire.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;
static int kinds[3];

void handle_contract_violation(const contract_violation_t* cv)
{
  if (handler_called < 3)
    kinds[handler_called] = stdc_contract_violation_kind(cv);
  handler_called++;
}

int compute(int x) _Pre(x > 0) _Post(r: r > 0)
{
  _ContractAssert(x != 0);
  return -x;  /* Bug: returns negative, violating postcondition.  */
}

int main(void)
{
  /* x == -1 violates pre; x != 0 in assert is satisfied;
     return 1 satisfies post.  Actually, let us trigger all three:
     pass -1 so pre fires, assert(x != 0) is true for -1,
     return -(-1) == 1 so post is satisfied.
     We need all three to fire.  Pass 0 instead:
     pre(0 > 0) fires, assert(0 != 0) fires, return -0 == 0
     so post(0 > 0) fires.  */
  compute(0);

  if (handler_called != 3)
    __builtin_abort();
  if (kinds[0] != STDC_CONTRACT_PRE)
    __builtin_abort();
  if (kinds[1] != STDC_CONTRACT_ASSERT)
    __builtin_abort();
  if (kinds[2] != STDC_CONTRACT_POST)
    __builtin_abort();

  return 0;
}
