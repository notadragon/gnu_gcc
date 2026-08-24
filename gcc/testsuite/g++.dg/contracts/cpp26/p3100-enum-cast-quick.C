// P3100: enum-cast out of range configured to "quick_enforce": terminates, no
// handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-enum-cast-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "enum-cast quick_enforce terminates" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
enum E { A, B, C };   // [dcl.enum] value range [0,3]
__attribute__((noinline)) E cast_it (int v) { return static_cast<E> (v); }
int main () { E e = cast_it (5); std::printf ("RESULT=%d\n", (int) e); return 0; }
