// A result binding is a local entity and is captured by a lambda in the
// postcondition that names it; the lambda reads the returned object, and in
// constant evaluation the call is a constant expression.
//
// Mirror: clang/test/Contracts/result-binding-lambda-capture.cpp in the
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

int f1 () post (r: [&] { return r; } () == 5) { return 5; }
int f2 () post (r: [=] { return r; } () == 5) { return 5; }
int f3 () post (r: [&] { return r; } () == 6) { return 5; }	// violation

constexpr int c1 () post (r: [&] { return r; } () == 5) { return 5; }
static_assert (c1 () == 5);

int main ()
{
  f1 ();
  f2 ();
  if (handled != 0)
    __builtin_abort ();
  f3 ();
  if (handled != 1)
    __builtin_abort ();
}
