// P4405, DECISIONS.md E169: p4405-nested-caller-side.C with runtime
// ignore, so that f's predicate is only analyzed, at each call: the same
// diagnostics at the calls, and none in f.
//
// Mirror: clang/test/Contracts/p4405-nested-caller-side.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }

int g (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
				// { dg-message "precondition of a call in the predicate is always false" "" { target *-*-* } .-1 }
				// { dg-bogus "the predicate" "" { target *-*-* } .-2 }
int g (int) { return 1; }

int f (int y)
  pre (g (y) == 1)		// { dg-bogus "cannot be proven" }
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

// E169 (7): the assertion's analysis evaluation, earlier in the proof
// shadow, evaluates `g (y)' -- g's own enforce check of its precondition,
// inlined there, included -- which is a premise for the evaluation at the
// same call in the check, as any code before it is:
// with runtime ignore there is no check.
int
checked_outer (int y)
{
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    g (y) == 1);		// { dg-bogus "cannot be proven" }
  return y;
}
