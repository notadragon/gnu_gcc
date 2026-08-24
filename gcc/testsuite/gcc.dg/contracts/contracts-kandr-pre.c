/* Test _Pre on an old-style (K&R) function definition.  Regression test:
   pending_contracts/pending_contract_tokens are file-static globals
   populated while parsing the declarator; parsing each old-style parameter
   declaration below recurses back into the same declaration parser, whose
   non-defining-declaration cleanup unconditionally truncated those globals
   -- silently discarding the enclosing function's own not-yet-injected
   contract before it was ever checked.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -Wno-old-style-definition" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  handler_called++;
}

int guarded(a, b) _Pre(a > 0)
     int a;
     int b;
{
  return a + b;
}

int main(void)
{
  if (guarded(1, 2) != 3)
    __builtin_abort();
  if (handler_called != 0)
    __builtin_abort();

  guarded(-1, 2);
  if (handler_called != 1)
    __builtin_abort();

  return 0;
}
