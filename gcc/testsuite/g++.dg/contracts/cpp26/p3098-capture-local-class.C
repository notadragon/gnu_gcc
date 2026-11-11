// P3098: a postcondition capture on a local-class member function is usable
// in the predicate; only the enclosing function's own locals are not
// (postcondition-outer-local.C).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

int main ()
{
  struct L {
    static int g (int x) post [c = x] (c == 1) { return x; }
  };
  L::g (1);
  if (violations != 0)
    __builtin_abort ();
  L::g (2);
  if (violations != 1)
    __builtin_abort ();
}
