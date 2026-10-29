// P4405: every evaluation with the prove semantic is an error until
// something can be proven -- a precondition at each call (D4405), a
// postcondition and an assertion in the function body.  -fmax-errors=6
// (the number expected) makes a repeated error fail the test: a seventh
// would end the compilation.
//
// Mirror: clang/test/Contracts/p4405-prove.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -fmax-errors=6" }

int f (int x)
  pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
  post (r: r > 0)		// { dg-error "contract assertion cannot be proven" }
{
  contract_assert (x != 3);	// { dg-error "contract assertion cannot be proven" }
  return x;
}

void g (int x)
{
  contract_assert (x > 0);	// { dg-error "contract assertion cannot be proven" }
  f (x);			// { dg-error "contract assertion cannot be proven" }
}

// A destructor's contract assertion is diagnosed once, not once per clone,
// and a constructor's precondition once per call.  A virtual base makes the
// complete and base clones two bodies, and gives the constructor an
// in-charge parameter its complete object clone, which the call calls, has
// not: the argument is still substituted for the parameter (GCC-917).
struct V {};
struct S : virtual V {
  int v;
  S (int x)
    pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
    : v (x) {}
  ~S () { contract_assert (v > 0); }	// { dg-error "contract assertion cannot be proven" }
};

S s (1);			// { dg-bogus "cannot be proven" }

void make (int y)
{
  S t (y);			// { dg-error "contract assertion cannot be proven" }
}
