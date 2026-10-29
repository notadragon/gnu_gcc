// D4405 ("The Analysis Evaluation"): a closure passed to std::function or
// std::invoke is not converted to a pointer; the library calls its function
// call operator directly, and that call has the analysis evaluation of the
// operator's preconditions, with arguments the library does not know.  So
// an always_prove precondition cannot be proven there -- an error, located
// in the library -- and an always_diagnose one draws a warning there (which
// the testsuite's library headers, not being system headers, show).  A
// closure passed to a forwarding template of the program's own is analyzed
// at the template's call in the same way.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <functional>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

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
