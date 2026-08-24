// P3100: this is a MIDDLE-END check, so throwing enforce/observe are excluded
// from its allowed set; without -fcontracts-p4298 a configured "enforce" is
// clamped to quick_enforce (a trap), NOT the throwing handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-align-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "enforce without p4298 clamps to quick_enforce (trap)" }
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
