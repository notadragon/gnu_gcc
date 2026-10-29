// P4405 stage 3: p4405-shadow-predicate-call.C with
// -fno-contracts-prove-assume: the call in g's precondition is still not
// diagnosed.
//
// Mirror: clang/test/Contracts/p4405-shadow-predicate-call.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -fno-contracts-prove-assume" }
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
