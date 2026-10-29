// P3100: the float-cast guard evaluates its operand exactly once, and the
// operand's side effects still happen on the violation path.  ignore semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-fpint-ignore.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int f_calls = 0;
double F (double v) { ++f_calls; return v; }

namespace ign_ns {
  int fi (double x) { return (int) F (x); }
}

int main () {
  // Violation path (out of range): operand evaluated exactly once.
  f_calls = 0;
  if (ign_ns::fi (1e30) != 0) __builtin_abort ();
  if (f_calls != 1) __builtin_abort ();

  // Normal path: operand evaluated exactly once.
  f_calls = 0;
  if (ign_ns::fi (42.9) != 42) __builtin_abort ();
  if (f_calls != 1) __builtin_abort ();
}
