// P3100: signed integer overflow, quick_enforce -- traps on overflow.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-overflow-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "signed overflow quick_enforce traps" }

#include <contracts>
#include <climits>

__attribute__((noinline)) int add (int a, int b) { return a + b; }

int main () { return add (INT_MAX, 1); }
