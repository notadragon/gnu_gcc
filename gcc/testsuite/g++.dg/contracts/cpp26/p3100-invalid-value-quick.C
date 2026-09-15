// P3100: invalid bool value load, quick_enforce -- traps.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-invalid-value-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "invalid value quick_enforce traps" }

#include <cstring>

__attribute__((noinline)) bool load (const bool* p) { return *p; }

int main () {
  unsigned char c = 4;
  bool b; __builtin_memcpy (&b, &c, 1);
  return load (&b);
}
