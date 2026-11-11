// P4405, DECISIONS.md E169: p4405-nested-conditional.C in a -O0 build,
// which proves at the default proof level, 2: the nested precondition and
// the assertion holding it are proven conditionally in the proof shadow,
// as at -O2.  -O0 drops the assumptions, so nothing is assumed.
//
// Mirror: clang/test/Contracts/p4405-nested-conditional.cpp in the
// llvm_llvm-project fork (its -O0 run).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontracts-prove-assume" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fdump-tree-contract_shadow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-nested-conditional.json" }

#include "p4405-nested-conditional.C"

// { dg-final { scan-tree-dump {\(line 25\): nested in} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 25\), with those nested in it: conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump-not {\(line 25\)[^\n]*: sound} "contract_shadow" } }
// { dg-final { contracts-same-diagnostics "" "-O2" } }
