// P3100: an IN-range enum-cast is not a violation even under "enforce" -- 3 is a
// valid value of enum E (range [0,3]) although no enumerator equals it.  No
// handler runs and the value is preserved.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-enum-cast-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation&) {
  std::printf ("UNEXPECTED-VIOLATION\n"); std::fflush (stdout);
}
enum E { A, B, C };
__attribute__((noinline)) E cast_it (int v) { return static_cast<E> (v); }
int main () { E e = cast_it (3); std::printf ("RESULT=%d\n", (int) e); return 0; }
// { dg-output "RESULT=3" }
