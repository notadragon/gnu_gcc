// -fcontract-group-evaluation-semantic= takes the semantic after the last
// ':', so a P3100 group, whose name always contains ':' ("ub:..."), can be
// named: the division by zero is observed and the program continues.
//
// Mirror: clang/test/Contracts/group-semantic-colon.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3100 -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

volatile int z = 0;

int main ()
{
  int l = 10 / z;
  if (violations != 1 || l != 0)
    __builtin_abort ();
}
