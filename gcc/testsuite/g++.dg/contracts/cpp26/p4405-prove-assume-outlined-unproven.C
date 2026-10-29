// P4405 -fcontracts-prove-assume (implied) with -fcontract-checks-outlined:
// an assumption made of an obligation that is not proven soundly is
// vacuous, and is removed from the real function before its late
// optimizations.  The outlined postcondition check reads `x' through a
// reference, so while the call stayed it kept `x' in memory, and `own'
// was compiled differently from analysis ignore (GCC-940).
// { dg-do assemble { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-checks-outlined -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -O2" }
// { dg-additional-options "-fdump-tree-contract_proven" }

int sink (int);

int own (const int x) post (r: r >= x) { return x + sink (x); }

// A soundly proven obligation's assumption stays.
int pos (int x)
{
  if (x > 0)
    contract_assert (x > 0);
  return x;
}

// { dg-final { scan-tree-dump "not proven soundly, its assumption removed" "contract_proven" } }
// The shadows' copies, and the real pos's.
// { dg-final { scan-tree-dump-times "\\.ASSUME \\(_Z3owni" 1 "contract_proven" } }
// { dg-final { scan-tree-dump-times "\\.ASSUME \\(_Z3posi" 2 "contract_proven" } }
// { dg-final { contracts-same-object-code "-fcontract-analysis-semantic=ignore" "" "-fno-contracts-prove-assume" } }
