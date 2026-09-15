// D4298: normal (non-terminating) execution paths for noexcept_observe and
// noexcept_enforce.  A non-throwing handler under noexcept_observe returns
// control normally after a violation, and a non-violating call under
// noexcept_enforce completes without invoking the handler or terminating.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4298" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4298-normal.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation&)
{
  ++violations;
}

// Contract on this line -> configured to noexcept_observe.
int observe_fn(int x) pre(x > 0) { return x; }

// Contract outside the listed line -> catch-all, configured to
// noexcept_enforce.
int enforce_fn(int x) pre(x > 0) { return x; }

int main()
{
  // noexcept_observe: violated, handler returns normally -> no termination.
  int r1 = observe_fn(-1);
  if (r1 != -1) __builtin_abort ();
  if (violations != 1) __builtin_abort ();

  // noexcept_enforce: predicate true, no violation, no handler call.
  int r2 = enforce_fn(1);
  if (r2 != 1) __builtin_abort ();
  if (violations != 1) __builtin_abort ();
}
