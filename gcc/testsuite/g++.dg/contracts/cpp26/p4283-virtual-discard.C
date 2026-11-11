// P4283 x P3097: a requires clause on a virtual function's contract is honored
// through the interface/implementation split -- when the constraint is
// unsatisfied for an instantiation the contract is discarded and the interface
// wrapper omits it, so no violation can occur on a call through the interface.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4283 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <concepts>
#include <contracts>

static int viol = 0;
void handle_contract_violation (const std::contracts::contract_violation &) {
  ++viol;
}

template <class T>
struct Base {
  virtual void f (T x) pre requires (std::integral<T>) (x > 0) { }
  virtual ~Base () = default;
};

int main ()
{
  Base<int> bi;
  Base<double> bd;
  Base<int> *pi = &bi;
  Base<double> *pd = &bd;

  // integral: contract active -> a violating call fires (via the P3097
  // two-source interface + implementation checks).
  viol = 0; pi->f (-1);   if (viol == 0) __builtin_abort ();

  // double: constraint unsatisfied -> contract discarded, wrapper omits it ->
  // a violating call fires nothing.
  viol = 0; pd->f (-1.0); if (viol != 0) __builtin_abort ();
}
