// P3100: [[assume]] with quick_enforce -- a false side-effect-free predicate
// fails fast (traps/terminates).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-assume-attr-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "assume quick_enforce" }

#include <contracts>
#include <cstdlib>

// quick_enforce never calls the handler: a call is a failure, and
// exiting normally is how a dg-shouldfail test reports one.
void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

__attribute__((noinline)) int f (int x) { [[assume (x > 0)]]; return x; }

int main () { return f (-1); }
