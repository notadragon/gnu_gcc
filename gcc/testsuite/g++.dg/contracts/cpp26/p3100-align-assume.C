// P3100: misaligned access under "assume" (the default): no check emitted, the
// raw access is performed -- byte-identical to a build without -fcontracts-p3100.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
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
// { dg-output "RESULT=" }
