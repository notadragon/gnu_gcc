// D4405 ("The Analysis Evaluation"): a closure passed to std::function or
// std::invoke is not converted to a pointer; the library calls its function
// call operator directly, and that call has the analysis evaluation of the
// operator's preconditions, with arguments the library does not know.  So
// an always_prove precondition cannot be proven there -- an error, located
// in the library -- and an always_diagnose one draws a warning there.  The
// library headers are system headers, but the precondition is this file's,
// so the warning is shown without -Wsystem-headers (notadragon_wg21
// DECISIONS.md E186 (a)), with the instantiation trail into this file.  A
// closure passed to a forwarding template of the program's own is analyzed
// at the template's call in the same way.  p4405-analysis-system-header.C
// covers a system header of the testsuite's own, and an assertion that is
// itself in a system header.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <functional>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

template<typename F, typename... A>
void my_invoke (F &&f, A &&...a)
{
  f (static_cast<A &&> (a)...);			// { dg-warning "contract assertion cannot be proven" }
}

void
use ()
{
  auto lp = [] (int x) pre<always_prove> (x > 0) {};	// { dg-message "the predicate of the contract assertion" }
  std::function<void (int)> g1 = lp;		// { dg-bogus "address" }
  g1 (1);
  auto ld = [] (int x) pre<always_diagnose> (x > 0) {};	// { dg-message "the predicate of the contract assertion" }
  std::function<void (int)> g2 = ld;		// { dg-bogus "address" }
  std::invoke (ld, 2);				// { dg-bogus "address" }
  my_invoke (ld, 3);
  g2 (1);
}

// { dg-error "contract assertion cannot be proven" "" { target *-*-* } 0 }
// { dg-warning "contract assertion cannot be proven" "" { target *-*-* } 0 }
