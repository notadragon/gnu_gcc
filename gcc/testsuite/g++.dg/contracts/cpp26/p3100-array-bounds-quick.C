// P3100: array subscript out of bounds, quick_enforce -- fails fast.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-array-bounds-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "array bounds quick_enforce" }

int g[4] = { 1, 2, 3, 4 };
__attribute__((noinline)) int rd (int i) { return g[i]; }

int main () { return rd (100); }
