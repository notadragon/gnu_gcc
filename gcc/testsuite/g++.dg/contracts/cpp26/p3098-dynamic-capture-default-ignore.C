// A dynamically selected postcondition whose static (weak default) semantic
// is ignore still initializes its captures, and is checked, when the selector
// chooses a semantic that evaluates it.
//
// Mirror: clang/test/Contracts/p3098-dynamic-capture-default-ignore.cpp in
// the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3098-dynamic-capture-default-ignore.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

static int inits, observed;
struct G {
  G () { ++inits; }
  G (const G &) { ++inits; }
};

evaluation_semantic sel () { return evaluation_semantic::observe; }

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++observed;
}

int f (int i) post [g = G ()] (false) { return i; }

int main ()
{
  f (1);
  if (inits != 1 || observed != 1)
    __builtin_abort ();
}
