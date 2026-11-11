// P4405 stage 3: with the proof shadow at -O2 the predicate is still never
// evaluated at run time -- the shadow is never emitted and the function's
// own code is ignore's -- whether the evaluation is proven or not, and a
// predicate's side effects stay out of the program.
//
// Mirror: clang/test/Contracts/Runnable/p4405-shadow-not-evaluated.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int evaluated, violations;
[[gnu::noinline]] bool touch (bool v) { ++evaluated; return v; }
[[gnu::noinline]] int bump () { return ++evaluated; }

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

[[gnu::noinline]] int f (const int x)
  pre (touch (x > 0))
  post (r: touch (r == x))
{
  if (x > 0)
    contract_assert (touch (x > 0) && x > 0);
  contract_assert (touch (false));
  contract_assert (bump () > 0);
  return x;
}

int main ()
{
  int r = f (1) + f (-1);
  if (r != 0 || evaluated != 0 || violations != 0)
    __builtin_abort ();
}
