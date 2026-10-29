// P4405: a caller-side evaluation with prove or diagnose is diagnosed at the
// call, once per call, with a note at the predicate; the callee-side ones
// here are ignore, so the definitions are not diagnosed.  A call in an
// unevaluated operand evaluates nothing and is not diagnosed.  Constructor
// and member function calls are diagnosed the same way, an inheriting
// constructor's call too, at that call (the inherited constructor's
// precondition).  -fmax-errors=5 makes a repeated error fail the test: a
// sixth error would end the compilation before the last expected one.  No
// argument is a constant, so
// folding with the arguments substituted (p4405-fold-caller-side.C) proves
// none of them.
//
// Mirror: clang/test/Contracts/p4405-caller-side.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fmax-errors=5" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-caller-side.json" }

int f (int x)
  pre (x > 0)		// { dg-message "the predicate of the contract assertion" }
  post (r: r > 0);	// { dg-message "the predicate of the contract assertion" }

int g (int x)
{
  return f (x);		// { dg-error "contract assertion cannot be proven" }
			// { dg-warning "contract assertion cannot be proven" "" { target *-*-* } .-1 }
}

decltype (f (1)) h ();	// { dg-bogus "cannot be proven" }

int k (int x)
{
  return f (x + 1);	// { dg-error "contract assertion cannot be proven" }
			// { dg-warning "contract assertion cannot be proven" "" { target *-*-* } .-1 }
}

struct S {
  S (int x) pre (x > 0);	// { dg-message "the predicate of the contract assertion" }
  void m (int x) pre (x > 0);	// { dg-message "the predicate of the contract assertion" }
};

// A call of an inheriting constructor evaluates the inherited constructor's
// precondition, diagnosed at that call and not at the using-declaration.
struct T : S {
  using S::S;		// { dg-bogus "cannot be proven" }
};

void use (int x)
{
  S s (x);		// { dg-error "contract assertion cannot be proven" }
  s.m (x);		// { dg-error "contract assertion cannot be proven" }
  T t (x);		// { dg-error "contract assertion cannot be proven" }
}
