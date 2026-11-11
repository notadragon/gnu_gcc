// P4405, DECISIONS.md E169 (5): when a predicate is also evaluated at run
// time (the assertions' runtime semantic is enforce here), its analysis
// evaluation depends on the nested preconditions as when it is only
// analyzed, and each nested precondition is analyzed at its call in the
// check as well, as at any call: two evaluations, two diagnostics.  At -O2
// the proof shadow decides the evaluation at the call in the check too,
// where its marker is placed in the check's copy of the predicate
// (GCC-953: it used to keep folding's result, so `guarded' reported g2's
// precondition).  The preconditions' own runtime semantic is ignore: an
// enforce check in g1, inlined into the shadow where the analysis
// evaluation evaluates `g1 (y)', would be a premise for the evaluation at
// the call in the check, as any code before it is, and prove it
// conditionally.  At -O0: p4405-nested-checked-O0.C.
//
// A nested diagnose precondition in a checked predicate (`report_once')
// is warned once at its call, though the nested evaluation and the one at
// the call in the check both make the warning (E169 (9)); the dg-regexp
// takes one warning, and a second would be an excess message.
//
// Mirror: clang/test/Contracts/p4405-nested-checked.cpp in the
// llvm_llvm-project fork (`report_once' too).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-nested-checked.json" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }
// { dg-additional-options "-fdump-tree-contract_shadow" }

int g1 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g1 (int) { return 1; }
int g2 (int x) pre (x != 0);	// { dg-bogus "precondition of a call" }
				// { dg-bogus "the predicate" "" { target *-*-* } .-1 }
int g2 (int) { return 1; }
int g3 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate is always false" }
int g3 (int x) { return 10 / x; }
int g4 (int x) pre (x != 0);	// { dg-bogus "precondition of a call" }
				// { dg-bogus "the predicate" "" { target *-*-* } .-1 }
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
  contract_assert (		// { dg-bogus "cannot be proven" }
    g2 (y) == 1);		// { dg-bogus "cannot be proven" }
  return y;
}

/* The evaluation at the call in the check is not diagnosed: folding finds
   it always false, but the proof shadow decides whether it is reached
   (DECISIONS.md E195), and the analysis evaluation's copy of g3 (0),
   earlier in the shadow, divides by zero, so it is not (E169 (7): a
   conditional proof).  At -O0 both are diagnosed:
   p4405-nested-checked-O0.C.  */
int
always_false (int y)
{
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    g3 (0) != 7);		// { dg-bogus "contract predicate is always false" }
  return y;
}

// The shape GCC-953 was found with: the outer predicate is not provable,
// but the nested precondition is, after the guard.
int
guarded_outer_unprovable (int y)
{
  if (y == 0)
    return 0;
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    g4 (y) != 7);		// { dg-bogus "cannot be proven" }
  return y;
}

namespace dd {
int g5 (int x) pre (x != 0);	// { dg-message "the predicate of the contract assertion" }
				// { dg-bogus "precondition of a call" "" { target *-*-* } .-1 }
int g5 (int) { return 1; }
}

// E169 (9): one warning at the call.  At -O2 the outer
// is proven, conditionally (E169 (10)).
int
report_once (int y)
{
  contract_assert (		// { dg-bogus "cannot be proven" }
    dd::g5 (y) == 1);
  return y;
}
// { dg-regexp {[^\n\r]*p4405-nested-checked\.C:95:[0-9]+: warning: contract assertion cannot be proven [^\n\r]*} }
// { dg-final { scan-tree-dump-not {\(line 53\): unproven} "contract_shadow" } }

// A -O0 build, at the default proof level, 2, gives the same diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
