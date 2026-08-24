/* Test multiple _Pre on one function: both fire independently.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  handler_called++;
}

int f(int x, int y) _Pre(x > 0) _Pre(y > 0)
{
  return x + y;
}

int main(void)
{
  /* Both preconditions violated.  */
  f(-1, -1);
  if (handler_called != 2)
    __builtin_abort();

  return 0;
}
