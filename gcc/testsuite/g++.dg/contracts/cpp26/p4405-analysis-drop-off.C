// D4405: p4405-analysis-drop.C with -fno-contracts-prove-assume: the proof
// of the contract_assert is not assumed, so its enforced check stays.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 -fno-contracts-prove-assume" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-analysis-drop.json" }

#include "p4405-analysis-drop.C"

// { dg-final { scan-tree-dump-times "__cxa_contract_violation_assert_enforce_pf \\(" 1 "optimized" } }
