// { dg-options "-fcontract-evaluation-semantic=enforce" }

#include <contracts>

int handled;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  throw 42;
}

[[gnu::noinline]] inline int
f (int x)
{
  contract_assert (x > 0);
  return x;
}

int (*volatile fp) (int) = f;
