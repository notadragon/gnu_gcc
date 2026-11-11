// As capture-partial-leak.C, with the checks outlined: a capture initialized
// before a later capture of the same postcondition threw is destroyed.
//
// Mirror: clang/test/Contracts/capture-partial-leak.cpp in the
// llvm_llvm-project fork (Clang has no outlined mode).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "capture-partial-leak.C"
