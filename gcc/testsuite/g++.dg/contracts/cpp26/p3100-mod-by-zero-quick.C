// P3100: modulo-by-zero parity with divide -- quick_enforce traps.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "modulo-by-zero quick_enforce traps" }

#include <contracts>

int m (int a, int b) { return a % b; }

int main () { return m (10, 0); }
