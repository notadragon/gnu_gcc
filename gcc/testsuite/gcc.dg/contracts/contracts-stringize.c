/* Test that the violation comment contains the predicate text.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>
#include <string.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  const char* comment = stdc_contract_violation_comment(cv);
  if (!comment)
    __builtin_abort();
  if (!strstr(comment, "x > 0"))
    __builtin_abort();
  handler_called++;
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
