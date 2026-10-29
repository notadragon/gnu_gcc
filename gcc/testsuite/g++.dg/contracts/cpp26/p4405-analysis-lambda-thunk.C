// D4405 ("The Analysis Evaluation"): a capture-less lambda converted to a
// pointer to function points to a static thunk that calls the function call
// operator with its own (unknown) arguments; that call is part of the
// indirect call and has no analysis evaluation -- for a generic lambda too,
// whose thunk is a template instantiated on conversion, and whether or not
// the operator was called directly before.  The preconditions here are
// analyzed only because the configuration says so, so the conversion is
// not diagnosed either ("Taking the Address of a Function"), and the
// direct calls are analyzed.  Unoptimized, so every analysis evaluation is
// decided at once.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontract-analysis-semantic=diagnose" }

int
use (int y)
{
  auto g = [] (auto x) pre (x > 0) { return x; };	// { dg-message "the predicate of the contract assertion" }
  int r = g (1);				// { dg-bogus "cannot be proven" }
  int (*p) (int) = g;				// { dg-bogus "address" }
  long (*pl) (long) = g;			// { dg-bogus "address" }
  auto n = [] (int x) pre (x > 0) { return x; };	// { dg-message "the predicate of the contract assertion" }
  int (*q) (int) = n;				// { dg-bogus "address" }
  r += n (y);					// { dg-warning "contract assertion cannot be proven" }
  r += g (y);					// { dg-warning "contract assertion cannot be proven" }
  return r + p (y) + (int) pl (y) + q (y);	// { dg-bogus "cannot be proven" }
}
