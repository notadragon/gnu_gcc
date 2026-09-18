// P3100: flowing off the end under "ignore" zeroes the bytes of the returned
// object regardless of its type -- no indeterminate data is leaked.  Covers
// non-int scalars and class/array returns, including a by-reference (sret)
// return and a by-value aggregate.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-ignore.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct Trivial { int a; long b; char c; };
struct Big { int a[8]; };                       // by-value aggregate
struct WithDtor { int a; long b; ~WithDtor () {} };   // non-trivial dtor -> sret
enum Color { RED = 5, BLUE = 9 };

int *fp (int x) { if (x > 0) return (int *) 1; }
bool fb (int x) { if (x > 0) return true; }
Color fe (int x) { if (x > 0) return BLUE; }
double fd (int x) { if (x > 0) return 3.5; }
Trivial ft (int x) { if (x > 0) return Trivial{1, 2, 3}; }
Big fbig (int x) { if (x > 0) return Big{{1, 2, 3, 4, 5, 6, 7, 8}}; }
WithDtor fw (int x) { if (x > 0) return WithDtor{1, 2}; }

int main () {
  if (fp (-1) != nullptr) __builtin_abort ();
  if (fb (-1) != false) __builtin_abort ();
  if (fe (-1) != (Color) 0) __builtin_abort ();
  if (fd (-1) != 0.0) __builtin_abort ();

  Trivial t = ft (-1);
  if (t.a != 0 || t.b != 0 || t.c != 0) __builtin_abort ();

  Big g = fbig (-1);
  for (int i = 0; i < 8; ++i)
    if (g.a[i] != 0) __builtin_abort ();

  WithDtor w = fw (-1);
  if (w.a != 0 || w.b != 0) __builtin_abort ();

  // Normal returns are unaffected.
  if (fp (1) == nullptr) __builtin_abort ();
  if (ft (1).a != 1) __builtin_abort ();
  if (fbig (1).a[7] != 8) __builtin_abort ();
}
