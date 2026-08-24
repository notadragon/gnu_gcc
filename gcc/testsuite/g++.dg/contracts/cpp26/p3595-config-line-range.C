// P3595 x config: a comma-separated location line list mixing a single line and
// a multi-line range.  Only a single contiguous range was previously covered.
// The config sets one single line and one range of this file to ignore, and
// everything else to observe.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-line-range.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// Contract on the single listed line -> ignore.
void f_single(int x) pre(x > 0) { }

// Contract inside the listed range -> ignore.
void f_in_range(int x)
  pre(x > 0)
{
}

// Contract outside both -> observe.
void f_outside(int x) pre(x > 0) { }

int main() {
  f_single(-1);     // ignore -> no handler
  if (violations != 0) __builtin_abort();
  f_in_range(-1);   // ignore -> no handler
  if (violations != 0) __builtin_abort();
  f_outside(-1);    // observe -> handler called
  if (violations != 1) __builtin_abort();
}
