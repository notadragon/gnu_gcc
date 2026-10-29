// { dg-options "-fcontract-evaluation-semantic=enforce -fcontracts-client-check=all" }

int f (int x) post (r: r > 0);

[[gnu::noinline]] inline int
g (int x)
{
  return f (x);
}

int
use_g (int y)
{
  return g (y) > 0;
}
