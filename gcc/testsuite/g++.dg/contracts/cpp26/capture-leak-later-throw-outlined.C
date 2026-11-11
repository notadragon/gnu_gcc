// capture-leak-later-throw.C with the checks outlined: the outlined pre
// function, which initializes the captures and checks the preconditions,
// destroys what it made when a later check throws.
//
// Mirror: clang/test/Contracts/capture-leak-later-throw.cpp in the
// llvm_llvm-project fork (Clang has no outlined mode).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "capture-leak-later-throw.C"
