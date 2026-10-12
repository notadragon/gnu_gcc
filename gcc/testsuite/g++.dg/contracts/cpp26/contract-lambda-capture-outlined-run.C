// A lambda's explicit capture named in its contracts, with the checks
// outlined: the capture proxy reads the capture through the closure
// parameter, which must be remapped to the outlined function's own.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

int main ()
{
  int k = 5;
  auto by_copy = [k] (const int x) pre (x != k) post (x == k) { return x; };
  auto by_ref = [&k] (const int x) pre (x != k) { return x; };

  by_copy (4);			// post fails
  if (violations != 1)
    __builtin_abort ();
  by_copy (5);			// pre fails
  if (violations != 2)
    __builtin_abort ();
  by_ref (4);
  k = 4;
  by_ref (4);			// pre fails
  if (violations != 3)
    __builtin_abort ();
}
