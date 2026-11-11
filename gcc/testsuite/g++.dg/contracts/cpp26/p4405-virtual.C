// P4405: the contracts of a virtual function (P3097) under prove.  A
// postcondition is diagnosed in the function's own body, and not again in
// the wrapper a virtual call goes through, which evaluates it again at run
// time with the callee's semantics.  A precondition is analyzed at each
// call (D4405) -- a virtual call too, for the function it names -- and not
// in the body or the wrapper.  -fmax-errors=2 (the number expected) makes a
// repeated error fail the test.
//
// Mirror: clang/test/Contracts/p4405-virtual.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -fmax-errors=2" }

struct B {
  virtual int h (int x)
    pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
    post (r: r > 1)		// { dg-error "contract assertion cannot be proven" }
  { return x; }
};

int call (B &b, int y)
{
  int r = b.h (1);		// { dg-bogus "cannot be proven" }
  return r + b.h (y);		// { dg-error "contract assertion cannot be proven" }
}
