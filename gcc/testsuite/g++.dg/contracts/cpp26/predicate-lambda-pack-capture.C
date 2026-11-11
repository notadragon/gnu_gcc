// A lambda in a function template's precondition or postcondition captures
// the template's function parameter pack when the template is instantiated
// (GCC-656; stock GCC rejects it in the template), as the same lambda in a
// contract_assert in the body does.
//
// Mirror: clang/test/Contracts/predicate-lambda-pack-capture.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

template <class... T>
int g (T... xs) pre ([=] { return ((xs > 0) && ...); } ()) { return 0; }
template <class... T>
int p (const T... xs) post ([&] { return ((xs > 0) && ...); } ()) { return 0; }
template <class... T>
int h (T... xs) { contract_assert ([=] { return ((xs > 0) && ...); } ()); return 0; }

int main ()
{
  g (1, 2);
  p (1, 2);
  h (1, 2);
  if (violations != 0)
    __builtin_abort ();
  g (1, -2);
  if (violations != 1)
    __builtin_abort ();
  p (-1, 2);
  if (violations != 2)
    __builtin_abort ();
  h (1, -2);
  if (violations != 3)
    __builtin_abort ();
}
