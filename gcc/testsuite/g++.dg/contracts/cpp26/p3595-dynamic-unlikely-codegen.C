// A dynamically selected check is marked unlikely in each arm that checks
// (notadragon_wg21 DECISIONS.md E63 (a)): observe, enforce and quick_enforce
// here; the ignore arm emits no check and the invalid-selection default is
// not a check.  contract-check-unlikely-codegen.C covers the static checks.
//
// Mirror: clang/test/Contracts/p3595-dynamic-unlikely-codegen.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fdump-tree-original" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-adjust-selector.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

void f (int x) pre (x > 0) { }

// { dg-final { scan-tree-dump-times "predicted unlikely by cold label predictor" 3 "original" } }
