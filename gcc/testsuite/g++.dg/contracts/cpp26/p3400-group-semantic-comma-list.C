// -fcontract-group-evaluation-semantic= accepts a comma-separated list of
// group:semantic elements: group "a" observes, group "b" is ignored.
//
// Mirror: clang/test/Contracts/group-semantic-comma-list.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-group-evaluation-semantic=a:observe,b:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
using namespace std::contracts::labels;

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++violations;
}

int f (int x) pre<"a.x"group> (x > 0) { return x; }
int g (int x) pre<"b.y"group> (x > 0) { return x; }

int main ()
{
  f (-1);
  g (-1);
  if (violations != 1)
    __builtin_abort ();
}
