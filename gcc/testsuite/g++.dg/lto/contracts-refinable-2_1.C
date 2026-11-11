// { dg-options "-fcontract-evaluation-semantic=ignore" }

[[gnu::noinline]] inline int
f (int x)
{
  contract_assert (x > 0);
  return x;
}

int
use_i (int y)
{
  return f (y) > 0;
}
