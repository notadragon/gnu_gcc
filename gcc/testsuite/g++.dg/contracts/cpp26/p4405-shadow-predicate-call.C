// P4405 stage 3, DECISIONS.md E169: a call in the predicate of a prove or
// diagnose evaluation is not evaluated by the program, but the evaluation
// depends on the analysis evaluations of its callee's preconditions all
// the same: here the precondition of g's call to check is caller-side
// prove, check's own caller-side precondition is prove and cannot be
// proven, so g's precondition is not proven, noting check's, which is not
// reported on its own -- at -O2, where the proof shadow (and, under
// -fcontracts-prove-assume, the assumption a sound proof may become) holds
// the predicate, as at -O0 (p4405-shadow-predicate-call-O0.C) and without
// -fcontracts-prove-assume (p4405-shadow-predicate-call-no-assume.C).  g's
// own callee-side evaluation is ignore.  A call in a predicate a check
// evaluates is also analyzed at the call (p4405-predicate-call-checked.C).
//
// Mirror: clang/test/Contracts/p4405-shadow-predicate-call.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-shadow-caller-side.json" }

bool check (int v)
  pre (v > 0);			// { dg-message "precondition of a call in the predicate cannot be proven" }
				// { dg-bogus "the predicate" "" { target *-*-* } .-1 }

void g (int v)
  pre (check (v))		// { dg-message "the predicate of the contract assertion" }
{}

void use (int v)
{
  g (v);			// { dg-error "contract assertion cannot be proven" }
}
