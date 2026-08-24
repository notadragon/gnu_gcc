// P3100: the divide-by-zero guard evaluates each operand exactly once, and the
// dividend's side effects still happen on the violation path (the divisor is
// evaluated by the guard condition; the dividend is forced too).  Covers both
// / and %.  ignore semantic (no handler; execution continues with 0).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-ignore.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int a_calls = 0, b_calls = 0;
int A () { ++a_calls; return 10; }
int B (int v) { ++b_calls; return v; }

int main () {
  // divide, violation path (divisor 0): both operands evaluated once, result 0.
  a_calls = b_calls = 0;
  if (A () / B (0) != 0) __builtin_abort ();
  if (a_calls != 1 || b_calls != 1) __builtin_abort ();

  // divide, normal path: operands evaluated once, result 5.
  a_calls = b_calls = 0;
  if (A () / B (2) != 5) __builtin_abort ();
  if (a_calls != 1 || b_calls != 1) __builtin_abort ();

  // modulo, violation path.
  a_calls = b_calls = 0;
  if (A () % B (0) != 0) __builtin_abort ();
  if (a_calls != 1 || b_calls != 1) __builtin_abort ();

  // modulo, normal path.
  a_calls = b_calls = 0;
  if (A () % B (3) != 1) __builtin_abort ();
  if (a_calls != 1 || b_calls != 1) __builtin_abort ();
}
