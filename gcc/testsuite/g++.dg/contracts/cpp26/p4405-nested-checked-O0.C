// P4405, DECISIONS.md E169 (5): p4405-nested-checked.C at proof level 0,
// folding alone: every outer assertion is unproven, noting its nested
// precondition unless that is proven, and every nested precondition is
// reported at its call in the check unless it is proven.
//
// A nested diagnose precondition in a checked predicate (`report_once')
// is warned once at its call, though the nested evaluation and the one at
// the call in the check both make the warning (E169 (9)); the dg-regexp
// takes one warning, and a second would be an excess message.
//
// Mirror: clang/test/Contracts/p4405-nested-checked.cpp in the
// llvm_llvm-project fork (`report_once' too).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0 -fcontracts-prove-optimize=0" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-nested-checked.json" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }

int g1 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g1 (int) { return 1; }
int g2 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g2 (int) { return 1; }
int g3 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate is always false" }
int g3 (int x) { return 10 / x; }
int g4 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g4 (int x) { return 10 / x; }

int
unguarded (int y)
{
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    g1 (y) == 1);		// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
guarded (int y)
{
  if (y == 0)
    return 0;
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    g2 (y) == 1);		// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
always_false (int y)
{
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    g3 (0) != 7);		// { dg-error "contract predicate is always false" }
  return y;
}

int
guarded_outer_unprovable (int y)
{
  if (y == 0)
    return 0;
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    g4 (y) != 7);		// { dg-error "contract assertion cannot be proven" }
  return y;
}

namespace dd {
int g5 (int x) pre (x != 0);	// { dg-message "the predicate of the contract assertion" }
				// { dg-bogus "precondition of a call" "" { target *-*-* } .-1 }
int g5 (int) { return 1; }
}

// E169 (9): one warning at the call.  At -O0 folding
// cannot prove the outer.
int
report_once (int y)
{
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    dd::g5 (y) == 1);
  return y;
}
// { dg-regexp {[^\n\r]*p4405-nested-checked-O0\.C:75:[0-9]+: warning: contract assertion cannot be proven [^\n\r]*} }
