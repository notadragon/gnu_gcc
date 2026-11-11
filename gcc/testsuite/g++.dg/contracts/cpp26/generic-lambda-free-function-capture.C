// A generic lambda in a free function's contract captures the parameter or
// result name it names.
//
// Mirror: clang/test/Contracts/generic-lambda-free-function-capture.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int handled = 0;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
}

int h (int x) pre ([&] (auto k) { return x + k; } (1) == 6) { return x; }
int f () post (r: [&] (auto k) { return r + k; } (1) == 6) { return 5; }

int main ()
{
  h (5);
  f ();
  if (handled != 0)
    __builtin_abort ();
}
