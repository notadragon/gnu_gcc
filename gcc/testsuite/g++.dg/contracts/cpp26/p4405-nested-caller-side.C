// P4405, DECISIONS.md E169: a caller-side precondition's analysis
// evaluation at a call depends on the preconditions of the calls in its
// predicate, with the call's arguments for the callee's parameters: `f (0)'
// makes g's precondition always false.  Here the predicate is also checked
// at run time on the callee's side (enforce), where g's precondition is
// analyzed at its call in f's check.  The shape crashed Clang's proof
// shadow at -O1/-O2 (llvm_llvm-project, PC2-Clang), which GCC never did.
// `checked_outer' pins E169 (7).  Runtime ignore:
// p4405-nested-caller-side-ignore.C; -O0: p4405-nested-caller-side-O0.C;
// -O1: p4405-nested-caller-side-O1.C; observe:
// p4405-nested-caller-side-observe{,-O1}.C.
//
// Mirror: clang/test/Contracts/p4405-nested-caller-side.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
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
// proven there at -O2 (GCC from -O2; llvm_llvm-project from -O1).
int
checked_outer (int y)
{
  contract_assert (		// { dg-error "contract assertion cannot be proven" }
    g (y) == 1);		// { dg-bogus "cannot be proven" }
  return y;
}

// A -O0 build, at the default proof level, 2, gives the same diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
