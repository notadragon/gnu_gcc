/* Test _Post on void function with side-effect checking (no result name).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  handler_called++;
}

static int value = 42;

void reset(int *p) _Post(*p == 0)
{
  /* Bug: does not actually reset *p.  */
  (void)p;
}

int main(void)
{
  reset(&value);
  if (handler_called != 1)
    __builtin_abort();

  return 0;
}
