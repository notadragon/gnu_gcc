// P4405 stage 3, -fcontracts-prove-shadow: as p4405-shadow-prove.C, with
// diagnose: an evaluation the proof shadow proves is not diagnosed, one it
// does not prove is a -Wcontract-unproven warning, with a note at the
// predicate when its location differs.
//
// Mirror: clang/test/Contracts/p4405-shadow-diagnose.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose" }

int f (int x)
  pre (x != 0)			// { dg-message "the predicate of the contract assertion" }
{
  if (x > 0)
    contract_assert (x != 0);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return x;
}

int g (int y)
{
  if (y < 0)
    return f (y);		// { dg-bogus "cannot be proven|always false" }
  return f (y);			// { dg-warning "contract assertion cannot be proven" }
}
