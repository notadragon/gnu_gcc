// P3100: a properly aligned access is NOT a violation, even under enforce.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-align-noexcept-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation&) {
  std::printf ("UNEXPECTED-VIOLATION\n"); std::fflush (stdout);
}
int __attribute__((noinline)) load (int* p) { return *p; }
int main () {
  alignas (int) char buf[8] = {1,2,3,4};
  int* p = reinterpret_cast<int*> (buf);       // properly aligned
  std::printf ("RESULT=%d\n", load (p));
  return 0;
}
// { dg-output "RESULT=" }
