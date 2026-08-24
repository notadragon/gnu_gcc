// P3100: divide-by-zero guard across integer types and both / and % (the
// distinct signed/unsigned and widened code paths).  ignore semantic: each
// zero-divisor op yields a defined 0; non-zero divisors compute normally.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-ignore.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace {
  unsigned du (unsigned a, unsigned b) { return a / b; }
  unsigned mu (unsigned a, unsigned b) { return a % b; }
  long dl (long a, long b) { return a / b; }
  long ml (long a, long b) { return a % b; }
  long long dll (long long a, long long b) { return a / b; }
  short ds (short a, short b) { return (short) (a / b); }
  char dc (char a, char b) { return (char) (a / b); }
  long dmix (long a, int b) { return a / b; }   // mixed operand widths
}

int main () {
  // Zero divisor -> defined 0, every type / operator.
  if (du (10u, 0u) != 0) __builtin_abort ();
  if (mu (10u, 0u) != 0) __builtin_abort ();
  if (dl (10L, 0L) != 0) __builtin_abort ();
  if (ml (10L, 0L) != 0) __builtin_abort ();
  if (dll (10LL, 0LL) != 0) __builtin_abort ();
  if (ds ((short) 10, (short) 0) != 0) __builtin_abort ();
  if (dc ((char) 10, (char) 0) != 0) __builtin_abort ();
  if (dmix (10L, 0) != 0) __builtin_abort ();

  // Non-zero divisor -> normal results.
  if (du (10u, 3u) != 3) __builtin_abort ();
  if (mu (10u, 3u) != 1) __builtin_abort ();
  if (dl (-10L, 3L) != -3) __builtin_abort ();
  if (ml (-10L, 3L) != -1) __builtin_abort ();
  if (dmix (100L, 7) != 14) __builtin_abort ();
}
