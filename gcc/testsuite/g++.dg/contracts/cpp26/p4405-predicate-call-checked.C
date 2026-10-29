// P4405: a call in a contract predicate that a check evaluates is
// evaluated, so its callee's caller-side prove and diagnose evaluations
// are diagnosed, at the call, where the check is emitted: here g's
// callee-side precondition and assertion are enforce, so g's body
// evaluates check (v) twice, and check's caller-side precondition is prove.
// g's own caller-side precondition is diagnosed at the call of g.  Where
// the predicate is never evaluated nothing is diagnosed
// (p4405-shadow-predicate-call.C).
//
// Mirror: clang/test/Contracts/p4405-predicate-call-checked.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fmax-errors=3" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-predicate-call-checked.json" }

bool check (int v)
  pre (v > 0);			// { dg-message "the predicate of the contract assertion" }

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
