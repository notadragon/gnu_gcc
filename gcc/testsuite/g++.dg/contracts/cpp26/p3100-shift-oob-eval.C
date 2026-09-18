// P3100: the shift guard evaluates each operand exactly once, and the shifted
// operand's side effects still happen on the violation path.  ignore semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type -Wno-shift-count-overflow -Wno-shift-count-negative" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-shift-oob-ignore.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int a_calls = 0, b_calls = 0;
int A () { ++a_calls; return 1; }
int B (int v) { ++b_calls; return v; }

int main () {
  // Violation path (count out of range): both operands evaluated once.
  a_calls = b_calls = 0;
  if ((A () << B (100)) != 0) __builtin_abort ();
  if (a_calls != 1 || b_calls != 1) __builtin_abort ();

  // Normal path.
  a_calls = b_calls = 0;
  if ((A () << B (4)) != 16) __builtin_abort ();
  if (a_calls != 1 || b_calls != 1) __builtin_abort ();

  // Negative count violation, >> too.
  a_calls = b_calls = 0;
  if ((A () >> B (-1)) != 0) __builtin_abort ();
  if (a_calls != 1 || b_calls != 1) __builtin_abort ();
}
