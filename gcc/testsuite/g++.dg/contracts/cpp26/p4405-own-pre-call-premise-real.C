// GCC-1003: an assumption region whose predicate calls functions leaves
// the real code unchanged.  The contract_assert evaluations are ignore; a
// shadow assumes their predicates, which call an opaque function and an
// inline accessor that is never inlined, and proves the prove evaluation
// after them conditionally (-fdump-tree-contract_shadow).  The region is
// gone from the real function before anything is emitted, so neither
// function is called, nor is the accessor emitted.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore -fno-contracts-prove-assume" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-optimized" }

#include <contracts>
using namespace std::contracts::labels;

int sink (int);
bool opaque (int);
[[gnu::noinline]] inline bool positive (int x) { return x > 0; }

int f (int x)
{
  contract_assert (opaque (x) && x > 1);
  contract_assert (positive (x));
  contract_assert<prove> (x > 1);
  return sink (x);
}

// { dg-final { scan-tree-dump {\(line 24\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump-not "opaque" "optimized" } }
// { dg-final { scan-tree-dump-not "positive" "optimized" } }
// { dg-final { scan-assembler-not "_Z6opaquei" } }
// { dg-final { scan-assembler-not "_Z8positivei" } }
