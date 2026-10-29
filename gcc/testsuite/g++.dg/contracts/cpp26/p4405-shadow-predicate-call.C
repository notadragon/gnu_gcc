// P4405 stage 3: a call in the predicate of a prove or diagnose evaluation
// is not evaluated by the program, so its own caller-side evaluations are
// not diagnosed -- at -O2, where the proof shadow (and, under
// -fcontracts-prove-assume, the assumption a sound proof may become) holds
// the predicate, as at -O0 (p4405-shadow-predicate-call-O0.C) and without
// -fcontracts-prove-assume (p4405-shadow-predicate-call-no-assume.C).  Here
// the precondition of g's call to check is caller-side prove, and check's
// own caller-side precondition is never diagnosed; g's own callee-side
// evaluation is ignore.  A call in a predicate a check evaluates is
// diagnosed (p4405-predicate-call-checked.C).
//
// Mirror: clang/test/Contracts/p4405-shadow-predicate-call.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-shadow-caller-side.json" }

bool check (int v)
  pre (v > 0);			// { dg-bogus "the predicate" }

void g (int v)
  pre (check (v))		// { dg-message "the predicate of the contract assertion" }
{}

void use (int v)
{
  g (v);			// { dg-error "contract assertion cannot be proven" }
}
