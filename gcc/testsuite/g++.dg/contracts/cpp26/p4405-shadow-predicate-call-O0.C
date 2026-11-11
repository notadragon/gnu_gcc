// P4405 stage 3, DECISIONS.md E169: p4405-shadow-predicate-call.C at -O0,
// where no proof shadow is made: g's precondition still depends on
// check's, which is noted and not reported on its own.
//
// Mirror: clang/test/Contracts/p4405-shadow-predicate-call.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
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
