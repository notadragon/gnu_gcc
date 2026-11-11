// P4405, DECISIONS.md E169: p4405-nested-caller-side.C with -O1, runtime
// enforce and -fcontracts-prove-optimize=follow, so that the proofs are made
// at level 1 (by default they are made at level 2 at -O1, as at -O2).
// GCC's level-1 pipeline proves less than its level-2 one, and less than
// llvm_llvm-project's: f's precondition at the guarded call is not proven
// (g's precondition there is, so nothing is noted), and `checked_outer's
// evaluation at the call in the check does not take the premise E169 (7)
// describes, so both are reported here, while llvm_llvm-project proves both
// at level 1.
//
// Mirror: clang/test/Contracts/p4405-nested-caller-side.cpp in the
// llvm_llvm-project fork (its -O1 enforce run).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O1 -fcontracts-prove-optimize=follow" }
// { dg-additional-options "-fcontract-evaluation-semantic=enforce" }
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
  return f (y);			// { dg-error "contract assertion cannot be proven" }
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
