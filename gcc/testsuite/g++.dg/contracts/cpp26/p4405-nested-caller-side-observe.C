// P4405, DECISIONS.md E169: p4405-nested-caller-side.C with runtime observe,
// -O2, no premise copies: as under enforce, but g's observe check is no
// premise, so `checked_outer's evaluation at the call in the check is reported.
//
// Mirror: clang/test/Contracts/p4405-nested-caller-side.cpp in the
// llvm_llvm-project fork (its -O2 observe run).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe -fno-contracts-prove-callee-premises" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }

int g (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
				// { dg-message "precondition of a call in the predicate is always false" "" { target *-*-* } .-1 }
				// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-2 }
int g (int) { return 1; }

int f (int y)
  pre (g (y) == 1)		// { dg-error "contract assertion cannot be proven" }
{
  return y;
}

int
caller (int y)
{
  return f (y);			// { dg-error "contract assertion cannot be proven" }
}

int
caller_guarded (int y)
{
  if (y == 0)
    return 0;
  return f (y);			// { dg-bogus "cannot be proven" }
}

int
caller_zero ()
{
  return f (0);			// { dg-error "contract assertion cannot be proven" }
}

// E169 (7): see p4405-nested-caller-side.C.
int
checked_outer (int y)
{
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    g (y) == 1);		// { dg-error "contract assertion cannot be proven" }
  return y;
}
