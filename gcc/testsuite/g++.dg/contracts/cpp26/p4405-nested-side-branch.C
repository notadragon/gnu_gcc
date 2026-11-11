// P4405, DECISIONS.md E169 (12): a nested obligation's region is a side
// branch of the path that never joins it again, so its failure is no
// premise for the other proofs in the same shadow: `y >= 0' after the
// guard (line 36, obligation 2) is proven soundly, and assumed under
// -fcontracts-prove-assume, next to g1's failed nested precondition
// (line 33), whose outer assertion is unproven.  (Before PC4-GCC every
// surviving marker made the shadow's other proofs conditional.)  E169
// (10) is kept: a nested diagnose precondition not proven (dd::g5's, line
// 45) makes its own outer (obligation 3) conditional, so not assumed,
// while the outer's own result and the proof after it (line 46,
// obligation 5) stay sound.
//
// Mirror: clang/test/Contracts/p4405-nested-side-branch.cpp in the
// llvm_llvm-project fork (-Rcontract-proof for the dumps).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-nested-diagnose.json" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }
// { dg-additional-options "-fcontracts-prove-assume -fdump-tree-contract_shadow -fdump-tree-contract_proven" }

int g1 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g1 (int) { return 1; }
namespace dd {
int g5 (int x) pre (x != 0);	// { dg-message "the predicate of the contract assertion" }
int g5 (int) { return 1; }
}
int sink (int);

int
side (int y)
{
  contract_assert (g1 (y) == 1);	// { dg-error "contract assertion cannot be proven" }
  if (y < 0)
    return 0;
  contract_assert (y >= 0);
  return sink (y);
}

int
side_diagnose (int y)
{
  if (y < 0)
    return 0;
  contract_assert (dd::g5 (y) == 1);	// { dg-warning "contract assertion cannot be proven" }
  contract_assert (y >= 0);
  return sink (y);
}

// { dg-final { scan-tree-dump {\(line 33\), with those nested in it: unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 36\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 45\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 45\), with those nested in it: conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 46\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 2: assumed} "contract_proven" } }
// { dg-final { scan-tree-dump {contract obligation 5: assumed} "contract_proven" } }
// { dg-final { scan-tree-dump-not {contract obligation 3: assumed} "contract_proven" } }

// The same diagnostics in a -O0 build, which proves at the proof level too
// (its shadows find these proofs conditional, not sound: -O0 computes no
// side-effect flags for the real callees, and drops the assumptions).
// { dg-final { contracts-same-diagnostics "" "-O0" } }
