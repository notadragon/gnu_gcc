// P3100: float-to-integer conversion out of range, quick_enforce -- traps, no
// handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-fpint-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "float-cast out of range quick_enforce traps" }

#include <contracts>

int fi (double x) { return (int) x; }

int main () { return fi (1e30); }
