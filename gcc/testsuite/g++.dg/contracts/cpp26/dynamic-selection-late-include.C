// Under P3595 dynamic selection, genericizing f builds the selector's
// declaration before <contracts> is seen; that does not declare anything in
// std::contracts, so the header's evaluation_semantic is accepted later.
//
// Mirror: clang/test/Contracts/dynamic-selection-late-include.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/dynamic-selection-late-include.json" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

int f (int x) pre (x > 0) { return x; }
int g (int x) { return f (x); }

#include <contracts>

int h () { return g (1); }
