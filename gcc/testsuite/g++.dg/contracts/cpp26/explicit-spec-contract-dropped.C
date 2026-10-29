// An explicit specialization declared with a contract and defined without
// one keeps the contract: the definition inherits it like any redeclaration.
//
// Clang: clang/test/Contracts/explicit-spec-contract-dropped.cpp.
//
//
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int reported = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{ ++reported; }

template <class T> void ft (T x);
template <> void ft<int> (int x) pre (x > 5);
template <> void ft<int> (int x) {}

int main ()
{
  ft (1);
  if (reported != 1)
    __builtin_abort ();
}
