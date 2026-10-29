// A block-scope function declaration that is the first declaration carries a
// precondition, and the later namespace-scope definition omits it, which is
// allowed: the call is checked (GCC-551).  The repeated-contract shape is
// block-scope-decl-then-def-contract.C.
//
// Mirror: clang/test/Contracts/block-scope-first-decl-contract.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int reported = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{ ++reported; }

int main ()
{
  void lx (int x) pre (x > 0);
  lx (-1);
  if (reported != 1)
    __builtin_abort ();
}

void lx (int) {}
