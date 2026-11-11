// P4405: a call in a contract predicate that a check evaluates is
// evaluated, so its callee's caller-side prove and diagnose evaluations
// are diagnosed, at the call, where the check is emitted: here g's
// callee-side precondition and assertion are enforce, so g's body
// evaluates check (v) twice, and check's caller-side precondition is prove.
// g's own caller-side precondition is diagnosed at the call of g, noting
// check's, on which it depends (DECISIONS.md E169).  h's checks evaluate
// check (v) only after `v > 5', which folding cannot use: at -O2 the proof
// shadow proves check's precondition there
// (p4405-predicate-call-checked-O2.C).  Where the predicate is never
// evaluated, check's precondition is not reported on its own
// (p4405-shadow-predicate-call.C).
//
// Mirror: clang/test/Contracts/p4405-predicate-call-checked.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fmax-errors=5 -fcontracts-prove-optimize=0" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-predicate-call-checked.json" }

bool check (int v)
  pre (v > 0);			// { dg-message "the predicate of the contract assertion" }
				// { dg-message "precondition of a call in the predicate cannot be proven" "" { target *-*-* } .-1 }

void g (int v)
  pre (check (v))		// { dg-message "the predicate of the contract assertion" }
				// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }
{
  contract_assert (check (v));	// { dg-error "contract assertion cannot be proven" }
}

void use (int v)
{
  g (v);			// { dg-error "contract assertion cannot be proven" }
}

void h (int v)
  pre (v > 5 && check (v))	// { dg-error "contract assertion cannot be proven" }
{
  contract_assert (v > 5 && check (v));	// { dg-error "contract assertion cannot be proven" }
}
