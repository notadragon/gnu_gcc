// P3595: test location matching (filename suffix + line ranges) in JSON config.
// Config sets lines 23-26 of this file to ignore, everything else to observe.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-location.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// This function's contract is outside lines 23-26 -> observe
void f_outside(int x)
  pre(x > 0)
{
}

// This function and its contract (lines 23-26) are in the ignore range.
void f_ignored(int x)
  pre(x > 0)  // in range 23-26 -> ignore
{
}

int main() {
  // f_ignored's contract is in the ignore range: no handler call
  f_ignored(-1);
  if (violations != 0) __builtin_abort();

  // f_outside's contract is outside the range: observe -> handler called
  f_outside(-1);
  if (violations != 1) __builtin_abort();
}
