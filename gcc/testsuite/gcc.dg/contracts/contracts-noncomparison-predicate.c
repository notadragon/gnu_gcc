/* Test _Pre/_Post/_ContractAssert with a non-comparison predicate (a bare
   pointer/call/flag, not wrapped in an explicit comparison).  Regression
   test: the negated condition used to be built by folding before converting
   to a truth value (the reverse of every other condition in the C front
   end), which ICEd in the gimplifier for any predicate that wasn't already
   a comparison.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  handler_called++;
}

static int check(int x)
{
  return x;
}

void by_pointer(int *p) _Pre(p)
{
}

int by_call(int x) _Post(r: check(r))
{
  return x;
}

int main(void)
{
  int ok = 1;

  by_pointer(&ok);
  if (handler_called != 0)
    __builtin_abort();

  by_pointer((int *) 0);
  if (handler_called != 1)
    __builtin_abort();

  by_call(1);
  if (handler_called != 1)
    __builtin_abort();

  by_call(0);
  if (handler_called != 2)
    __builtin_abort();

  _ContractAssert(check(1));
  if (handler_called != 2)
    __builtin_abort();

  _ContractAssert(check(0));
  if (handler_called != 3)
    __builtin_abort();

  return 0;
}
