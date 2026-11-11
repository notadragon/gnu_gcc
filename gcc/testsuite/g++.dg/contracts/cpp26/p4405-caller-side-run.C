// P4405: a caller-side diagnose has no run-time effect: its predicate is not
// evaluated at the call, even when a wrapper is built there for another
// contract of the callee (here an observed postcondition), and it is
// diagnosed once, at the call, not again in the wrapper.
//
// Mirror: clang/test/Contracts/p4405-caller-side-run.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-caller-side-run.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int pre_evaluated, post_evaluated, violations;
bool pre_chk (int x) { ++pre_evaluated; return x > 0; }
bool post_chk (int r) { ++post_evaluated; return r > 0; }

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

int f (int x) pre (pre_chk (x)) post (r: post_chk (r)) { return x; } // { dg-message "the predicate of the contract assertion" }

int main ()
{
  f (-1);		// { dg-warning "contract assertion cannot be proven" }
  if (pre_evaluated != 0 || post_evaluated != 1 || violations != 1)
    __builtin_abort ();
}
