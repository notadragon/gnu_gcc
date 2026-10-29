// D4405 ("Taking the Address of a Function", DECISIONS.md E144 (a)): under
// -fcontract-analysis-semantic=prove every precondition is analyzed with
// prove, but one without a label allows runtime semantics, so taking its
// address -- including binding it to the forwarding reference of
// std::invoke or std::function -- is not diagnosed: this translation
// unit's configuration does not determine what calls through the address
// are.  Its calls through the pointer go unanalyzed and are still checked
// by the callee; a direct call is analyzed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <functional>

void fn (int x) pre (x > 0);			// { dg-bogus "precondition here" }
void fq (int x);

void
use ()
{
  std::invoke (fn, 1);				// { dg-bogus "address" }
  std::function<void (int)> g = fn;		// { dg-bogus "address" }
  void (*p) (int) = fn;				// { dg-bogus "address" }
  std::invoke (fq, 1);				// { dg-bogus "address|analyze" }
  fn (1);					// { dg-bogus "cannot be proven|address" }
  g (1);
  p (1);
}
