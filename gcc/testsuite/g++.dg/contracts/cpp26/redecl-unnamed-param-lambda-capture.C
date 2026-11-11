// A contract on a first declaration whose lambda captures a parameter, with
// a definition that leaves that parameter unnamed (allowed).  The contract
// must still be checked, in a template too (redecl-omit-contract-lambda.C
// has the templated shapes with a named parameter).
//
// Mirror: clang/test/Contracts/redecl-unnamed-param-lambda-capture.cpp
// in the llvm_llvm-project fork, which also covers the templated shapes.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

int f (int x) pre ([&] { return x > 0; } ());
int f (int) { return 0; }

int g (int x) pre ([=] { return x > 0; } ());
int g (int) { return 0; }

struct B { int h (int x) pre ([&] { return x > 0; } ()); };
int B::h (int) { return 0; }

template <class T> struct C { int h (int x) pre ([&] { return x > 0; } ()); };
template <class T> int C<T>::h (int) { return 0; }

int main ()
{
  f (1);
  g (1);
  B{}.h (1);
  C<int>{}.h (1);
  if (violations != 0)
    __builtin_abort ();
  f (-1);
  g (-1);
  B{}.h (-1);
  C<int>{}.h (-1);
  if (violations != 4)
    __builtin_abort ();
}
