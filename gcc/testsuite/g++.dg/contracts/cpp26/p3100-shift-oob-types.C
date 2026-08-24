// P3100: the shift out-of-range predicate uses the promoted left operand's
// width, so the same shift amount is UB for a narrower type but valid for a
// wider one.  ignore semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type -Wno-shift-count-overflow -Wno-shift-count-negative" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-shift-oob-ignore.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace {
  int si (int a, int b) { return a << b; }
  unsigned su (unsigned a, int b) { return a << b; }
  long sl (long a, int b) { return a << b; }
  long long sll (long long a, int b) { return a << b; }
}

int main () {
  // 40 >= width(int)==32 -> UB for int -> 0; but 40 < width(long)==64 -> valid.
  if (si (1, 40) != 0) __builtin_abort ();
  if (sl (1L, 40) != (1L << 40)) __builtin_abort ();
  if (sll (1LL, 40) != (1LL << 40)) __builtin_abort ();

  // unsigned width is 32.
  if (su (1u, 100) != 0) __builtin_abort ();
  if (su (1u, 5) != 32u) __builtin_abort ();

  // In-range for each.
  if (si (1, 5) != 32) __builtin_abort ();
  if (sl (3L, 2) != 12L) __builtin_abort ();
}
