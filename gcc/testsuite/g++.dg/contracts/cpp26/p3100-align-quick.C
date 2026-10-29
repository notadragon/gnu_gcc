// P3100: misaligned access configured to "quick_enforce": traps, no handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-align-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "misaligned quick_enforce traps" }
#include <contracts>
#include <cstdlib>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  /* quick_enforce never calls the handler: a call is a failure, and
     exiting normally is how a dg-shouldfail test reports one.  */
  (void) v;
  std::_Exit (0);
}
int __attribute__((noinline)) load (int* p) { return *p; }
int main () {
  alignas (int) char buf[8] = {1,2,3,4,5,6,7,8};
  int* p = reinterpret_cast<int*> (buf + 1);   // 1 byte off -> misaligned
  std::printf ("RESULT=%d\n", load (p));
  return 0;
}
