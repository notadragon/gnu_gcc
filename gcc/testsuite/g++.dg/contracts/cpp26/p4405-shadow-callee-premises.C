// D4405R0 Section 4.5, notadragon_wg21 DECISIONS.md E196 item 2, E199 (b):
// an observe, noexcept_observe or ignore evaluation is a premise that makes
// a later proof conditional, and a conditional proof is not diagnosed --
// also when the evaluation is in a callee the proof shadow inlines
// (-fcontracts-prove-callee-premises, implied by -fcontracts-p4405).  The
// callee's premise copy, which assumes its observe evaluations, stands for
// it in the premise shadow, which decides again what the ordinary shadow
// left unproven: a postcondition (h1), a contract_assert in an inline
// callee (h3), a callee reached through a function without contract
// assertions (h4: the copy of k calls the copy of idx), and a precondition
// in the callee's body (h5).  h2's premise is in its own body, which the
// ordinary shadow assumes with or without the option.  Each is proven
// conditionally (-fdump-tree-contract_shadow); with
// -fno-contracts-prove-callee-premises h1, h3, h4 and h5 are errors
// (p4405-shadow-callee-premises-off.C).
//
// Mirror: clang/test/Contracts/p4405-shadow-callee-premises.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe -fdump-tree-contract_shadow" }

#include <contracts>
using namespace std::contracts::labels;

int idx (int x) post (r : r == 0) { return x; }
void h1 (int x)
{
  int i = idx (x);
  contract_assert<prove> (i == 0);	// { dg-bogus "cannot be proven" }
}

void h2 (int x)
{
  contract_assert (x == 0);
  contract_assert<prove> (x == 0);	// { dg-bogus "cannot be proven" }
}

inline int idx2 (int x)
{
  contract_assert (x == 0);
  return x;
}
void h3 (int x)
{
  int i = idx2 (x);
  contract_assert<prove> (i == 0);	// { dg-bogus "cannot be proven" }
}

int k (int x) { return idx (x) + 1; }
void h4 (int x)
{
  int i = k (x);
  contract_assert<prove> (i == 1);	// { dg-bogus "cannot be proven" }
}

void take (int x) pre (x == 0) {}
void h5 (int x)
{
  take (x);
  contract_assert<prove> (x == 0);	// { dg-bogus "cannot be proven" }
}

// { dg-final { scan-tree-dump {contract obligation [0-9]+ \(line 30\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 30\): proven, conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation [0-9]+ \(line 36\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump-not {callee premises: obligation [0-9]+ \(line 36\)} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 47\): proven, conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 54\): proven, conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 61\): proven, conditional} "contract_shadow" } }
