// P4405 / P3100: after an evaluation with the assume semantic, GCC's real
// code already knows the predicate -- assume is lowered as an optimizer
// assumption -- so x / 4 becomes a shift with -fno-contracts-prove-assume
// too, and the proof shadow proves the later prove evaluation soundly.  In
// Clang, which lowers assume like ignore (CLANG-111, DECISIONS.md E132),
// only the assumption made of the proof gives the shift: its
// p4405-prove-assume.cpp uses this case (sound ()) to show
// -fcontracts-prove-assume; p4405-prove-assume.C uses a shadow-only callee.
//
// Clang counterpart: clang/test/Contracts/p4405-prove-assume-off.cpp in the
// llvm_llvm-project fork, whose sound () is this case and keeps the
// division.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=prove" }
// { dg-additional-options "-fdump-tree-optimized -fdump-tree-contract_shadow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-prove-assume-after-assume.json" }

int sink (int);

int assumed (int x)
{
  contract_assert (x > 0);	// line 23: assume
  contract_assert (x > 0);	// line 24
  return sink (x / 4);
}

// { dg-final { scan-tree-dump-times " >> 2;" 1 "optimized" } }
// { dg-final { scan-tree-dump-not " / 4;" "optimized" } }
// { dg-final { scan-tree-dump-times "\\(line 24\\): sound" 1 "contract_shadow" } }
