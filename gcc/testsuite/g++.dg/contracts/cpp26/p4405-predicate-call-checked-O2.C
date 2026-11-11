// P4405: p4405-predicate-call-checked.C at -O2, where the proof shadow
// decides the analysis evaluations at the calls in the checks (GCC-953):
// none in h, whose checks evaluate check (v) only after `v > 5', which
// folding cannot use but the shadow can; and the one in g's assertion is
// proven (conditionally) by the one in g's precondition check before it,
// whose marker is a premise for what follows, as for any two calls of
// check in turn.
//
// Mirror: clang/test/Contracts/p4405-predicate-call-checked.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fmax-errors=5 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-predicate-call-checked.json" }

bool check (int v)
  pre (v > 0);			// { dg-message "the predicate of the contract assertion" }
				// { dg-message "precondition of a call in the predicate cannot be proven" "" { target *-*-* } .-1 }

void g (int v)
  pre (check (v))		// { dg-message "the predicate of the contract assertion" }
				// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }
{
  contract_assert (check (v));	// { dg-bogus "cannot be proven" }
}

void use (int v)
{
  g (v);			// { dg-error "contract assertion cannot be proven" }
}

void h (int v)
  pre (v > 5 && check (v))	// { dg-bogus "cannot be proven" }
{
  contract_assert (v > 5 && check (v));	// { dg-bogus "cannot be proven" }
}
