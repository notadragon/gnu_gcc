// { dg-options "-fcontract-evaluation-semantic=enforce" }

[[gnu::noinline]] inline int
f (int x)
{
  contract_assert (x > 0);
  return x;
}

int
use_e (int y)
{
  return f (y) > 0;
}
