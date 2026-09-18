// P3100 x P4298: misaligned access (ub:basic.align.object.alignment) configured
// to "noexcept_observe": the middle-end check reports (sem=6) through the nothrow
// handler and then PROCEEDS into the real (still-misaligned) access -- there is
// no defined substitute for the lvalue (like null-deref observe).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-align-noexcept-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
int __attribute__((noinline)) load (int* p) { return *p; }
int main () {
  alignas (int) char buf[8] = {1,2,3,4,5,6,7,8};
  int* p = reinterpret_cast<int*> (buf + 1);   // 1 byte off -> misaligned
  std::printf ("RESULT=%d\n", load (p));
  return 0;
}
// { dg-output "VIOL kind=7 sem=6 comment=\\\[misaligned pointer access\\\]\r*\nRESULT=" }
