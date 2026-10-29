// { dg-options "-fcontract-evaluation-semantic=ignore" }

[[gnu::noinline]] inline int
f (int x)
{
  contract_assert (x > 0);
  return x;
}

struct D { ~D (); };

int dtors;

D::~D () { ++dtors; }

int
use_catch (int y)
{
  try
    {
      return f (y);
    }
  catch (int)
    {
      return -1;
    }
}

int
use_dtor (int y)
{
  D d;
  return f (y);
}
