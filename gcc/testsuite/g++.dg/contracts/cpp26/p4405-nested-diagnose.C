// P4405, DECISIONS.md E169: a nested precondition analyzed with diagnose
// that is not proven gets its own warning, at its call in the predicate,
// though the predicate is never evaluated at run time (runtime ignore
// here); it does not make the outer assertion, analyzed with prove,
// unproven.  At -O2 the proof shadow proves the outer ones; the nested one
// is proven after a guard.  At -O0: p4405-nested-diagnose-O0.C.
//
// Mirror: clang/test/Contracts/p4405-nested-diagnose.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-nested-diagnose.json" }

namespace dd {
int g1 (int x) pre (x != 0);	// { dg-message "the predicate of the contract assertion" }
				// { dg-bogus "precondition of a call" "" { target *-*-* } .-1 }
int g1 (int) { return 1; }
int g2 (int x) pre (x != 0);	// { dg-bogus "the predicate" }
				// { dg-bogus "precondition of a call" "" { target *-*-* } .-1 }
int g2 (int) { return 1; }
}

int
unprovable (int y)
{
  contract_assert (		// { dg-bogus "cannot be proven" }
    dd::g1 (y) == 1);		// { dg-warning "contract assertion cannot be proven" }
  return y;
}

int
guarded (int y)
{
  if (y == 0)
    return 0;
  contract_assert (		// { dg-bogus "cannot be proven" }
    dd::g2 (y) == 1);		// { dg-bogus "cannot be proven" }
  return y;
}

// A -O0 build, at the default proof level, 2, gives the same diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
