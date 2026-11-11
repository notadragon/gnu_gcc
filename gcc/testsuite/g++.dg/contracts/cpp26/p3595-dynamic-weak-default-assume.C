// With -fcontracts-allow-assume, a dynamically selected contract whose
// configuration names assume as the default semantic gets a weak default
// selector that returns assume.  A selector may return assume: allowed, it
// takes the ignore path, so the predicate is not evaluated and nothing is
// reported (notadragon_wg21 DECISIONS.md E66; GCC-690 was an enforced
// violation here).  Without the flag assume is adjusted to ignore
// (p3595-dynamic-adjust-selector.C).
//
// Mirror: clang/test/Contracts/p3595-dynamic-weak-default-assume.cpp and
// clang/test/Contracts/C/c-dynamic-weak-default-assume.c in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-allow-assume" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-weak-default-assume.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations, evaluated;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

bool chk (int x) { ++evaluated; return x > 0; }

int f (int x) pre (chk (x)) { return x; }

int
main ()
{
  f (-1);
  return violations + evaluated;
}
