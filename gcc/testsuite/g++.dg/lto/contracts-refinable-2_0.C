// As contracts-refinable-1, with the units in the other link order: the
// linker keeps this unit's copy of f, which enforces the assertion, so the
// call from the other unit, which ignores it, runs the violation handler.
// { dg-lto-do run }
// { dg-lto-options { { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fno-ipa-sra } { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fno-ipa-sra -flto } } }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  std::exit (0);
}

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

int use_i (int);

int
main ()
{
  if (use_i (-5) != 0)
    __builtin_abort ();
  if (use_e (-5) != 0)
    __builtin_abort ();
}
