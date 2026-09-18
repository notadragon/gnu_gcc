// P3400: a label whose handle_contract_violation is not const does not
// provide the local-violation facet, and its handler must not run.
//
// The near-miss tests cover the warning.  This covers the consequence, which
// nothing else pins: the violation is reported by the default handler and the
// label's own function is never entered.
//
// That invariant is load-bearing for the front end.  The local-handler
// trampoline casts the label pointer to `const LabelType *` and calls through
// it; that is only sound because a non-const handler is rejected as a facet
// before any trampoline is built.  If facet detection ever stopped enforcing
// const, this test fails rather than the trampoline silently calling a
// non-const member through a const lvalue.

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

int called = 0;

struct label_t {
  using assertion_control_object = label_t;
  // Deliberately NOT const, so labels::local_violation_label is unsatisfied.
  violation_handled
  handle_contract_violation (const contract_violation&)  // { dg-message "declared here" }
  {
    ++called;
    return violation_handled::handled;
  }
};
constexpr label_t label{};

static_assert (!std::contracts::labels::local_violation_label<label_t>,
	       "a non-const handler must not satisfy the concept");

int f (int x) pre<label> (x > 0) { return x; }  // { dg-warning "does not provide the 'handle_contract_violation' facet" }

int
main ()
{
  f (-1);
  // The default handler reported it; the label's did not run.
  if (called != 0)
    __builtin_abort ();
  return 0;
}
