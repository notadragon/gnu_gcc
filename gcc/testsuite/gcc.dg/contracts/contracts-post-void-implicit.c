/* Test _Post on void function at implicit return (no explicit return stmt).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;
static int flag = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  handler_called++;
}

void set_flag(void) _Post(flag == 1)
{
  /* Bug: does not set flag.  Falls through to implicit return.  */
}

int main(void)
{
  set_flag();
  if (handler_called != 1)
    __builtin_abort();

  return 0;
}
