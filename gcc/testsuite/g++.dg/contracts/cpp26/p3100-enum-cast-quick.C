// P3100: enum-cast out of range configured to "quick_enforce": terminates, no
// handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-enum-cast-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "enum-cast quick_enforce terminates" }

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
enum E { A, B, C };   // [dcl.enum] value range [0,3]
__attribute__((noinline)) E cast_it (int v) { return static_cast<E> (v); }
int main () { E e = cast_it (5); std::printf ("RESULT=%d\n", (int) e); return 0; }
