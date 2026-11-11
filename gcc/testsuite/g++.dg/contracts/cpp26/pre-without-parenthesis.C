// A precondition or postcondition whose predicate is not parenthesized is
// a syntax error, diagnosed without further trouble.  So are such a
// postcondition after a well-formed precondition, a declaration abandoned
// after a well-formed precondition, a local function definition with a
// contract, and the same on member functions; parsing recovers, and a later
// declaration is diagnosed as usual (GCC-662).  A malformed label is
// contract-label-malformed-recovery.C.
//
// GCC recovers from `post x' by parsing it as `post (x)', so a non-const
// parameter named there is also diagnosed as a postcondition's odr-use.
//
// Mirror: clang/test/Contracts/pre-without-parenthesis.cpp in the
// llvm_llvm-project fork (CLANG-638: an assertion failure after the error).
// Found by the EDG fork's parse/errors test.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

void f (int x) pre x > 0;	// { dg-error "expected '\\(' before 'x'" }
// { dg-error "expected '\\)' before ';'" "" { target *-*-* } .-1 }
void g (int x) post x;		// { dg-error "expected '\\(' before 'x'" }
// { dg-error "expected '\\)' before ';'" "" { target *-*-* } .-1 }
// { dg-error "postcondition must be const" "" { target *-*-* } .-2 }
void h (int x) pre (x > 0) post x;	// { dg-error "expected '\\(' before 'x'" }
// { dg-error "expected '\\)' before ';'" "" { target *-*-* } .-1 }
// { dg-error "postcondition must be const" "" { target *-*-* } .-2 }
void i (int x) pre (x > 0) garbage;	// { dg-error "expected initializer before 'garbage'" }
void l ()
{
  void lf (int x) pre (x > 0) {}	// { dg-error "function-definition is not allowed here" }
}

struct S
{
  void m (int x) pre x > 0;	// { dg-error "expected '\\(' before 'x'" }
  // { dg-error "expected '\\)' before ';'" "" { target *-*-* } .-1 }
  void n (int x) pre (x > 0) post x;	// { dg-error "expected '\\(' before 'x'" }
  // { dg-error "expected '\\)' before ';'" "" { target *-*-* } .-1 }
  // { dg-error "postcondition must be const" "" { target *-*-* } .-2 }
};

int after = undeclared;		// { dg-error "'undeclared' was not declared in this scope" }
