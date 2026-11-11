// A variable declared in a GNU statement expression in a generic lambda's
// precondition or postcondition is a local of the operator() specialization
// it is substituted into (GCC-652; stock GCC too).  The specialization's
// contracts are substituted before its function is started, and the use
// that needs its return type instantiates it inside the function using it.
// A postcondition of a deduced return type is GCC-660
// (open-bug-stmt-expr-local-generic-lambda-deduced-post.C).
//
// Mirror: clang/test/Contracts/stmt-expr-local-generic-lambda.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

int k (int v)
{
  auto m = [] (auto x) pre (__extension__ ({ int y = x; y > 0; })) { return x; };
  return m (v);
}

int k2 (int v)
{
  auto m = [] (auto x) -> int post (r: __extension__ ({ int z = r; z > 0; })) { return x; };
  return m (v);
}

int main ()
{
  k (1);
  k2 (1);
  if (violations != 0)
    __builtin_abort ();
  k (-1);
  if (violations != 1)
    __builtin_abort ();
  k2 (-1);
  if (violations != 2)
    __builtin_abort ();
}
