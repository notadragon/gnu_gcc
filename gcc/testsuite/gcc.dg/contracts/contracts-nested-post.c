/* A _Post on a GNU nested function must not ICE, and its postcondition
   must be scoped to the nested function -- not evaluated with the
   enclosing function's postconditions (D4299).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  (void) cv;
  handler_called++;
}

int outer(int a) _Post(r: r > 0)
{
  int inner(int b) _Post(r: r > 0)
  {
    return b;
  }
  return inner (a);
}

int main(void)
{
  if (outer (5) != 5)
    __builtin_abort();
  /* Both postconditions hold for a positive argument.  */
  if (handler_called != 0)
    __builtin_abort();
  return 0;
}
