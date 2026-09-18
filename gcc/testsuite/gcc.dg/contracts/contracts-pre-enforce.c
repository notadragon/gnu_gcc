/* Test _Pre with enforce semantic: handler called, then terminated.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=enforce" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>
#include <stdlib.h>

void handle_contract_violation(const contract_violation_t* cv)
{
  /* Handler invoked; exit immediately to prevent std::terminate.  */
  _Exit(0);
}

int guarded(int x) _Pre(x > 0)
{
  return x;
}

int main(void)
{
  guarded(-1);
  /* Should not reach here.  */
  return 1;
}
