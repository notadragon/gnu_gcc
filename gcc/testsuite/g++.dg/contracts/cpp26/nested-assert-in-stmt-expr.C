// A contract_assert inside a GNU statement expression in another
// contract_assert's predicate is valid and evaluates both; the inner
// violation is reported.
//
// Mirror: clang/test/Contracts/nested-assert-in-stmt-expr.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int handled = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++handled;
}

int f (int x)
{
  contract_assert (__extension__ ({ contract_assert (x > 1); x > 0; }));
  return x;
}

int main ()
{
  f (2);
  if (handled != 0)
    __builtin_abort ();
  f (1);   // the nested assertion fails, the outer one holds
  if (handled != 1)
    __builtin_abort ();
}
