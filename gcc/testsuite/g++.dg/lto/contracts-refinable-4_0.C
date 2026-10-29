// As contracts-refinable-3, with the units in the other link order: the
// linker keeps this unit's copy of f, which ignores the assertion, so no
// exception is thrown.
// { dg-lto-do run }
// { dg-lto-options { { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fno-ipa-sra } { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fno-ipa-sra -flto } } }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

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

extern int handled;

int
main ()
{
  int r = use_catch (-5);
  if (handled ? r != -1 : r != -5)
    __builtin_abort ();
  int h = handled;
  try
    {
      r = use_dtor (-5);
      if (handled != h || r != -5)
	__builtin_abort ();
    }
  catch (int)
    {
      if (handled == h)
	__builtin_abort ();
    }
  if (dtors != 1)
    __builtin_abort ();
}
