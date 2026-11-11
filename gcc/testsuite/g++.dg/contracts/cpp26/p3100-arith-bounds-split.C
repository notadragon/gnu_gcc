// P3100: configuring only the .arithmetic leaf of
// ub:expr.add.out.of.bounds.known checks a + i, and leaves the subscript a[i]
// (the .subscript leaf) at the default "assume".
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation (const cs::contract_violation& v) {
  if (v.kind () == cs::assertion_kind::implicit)
    ++viols;
}
int a[3];
volatile int vi;
int main () {
  vi = 5;
  int s = &a[vi] - a;		// 5: not checked
  int p = (a + vi) - a;		// caught -> 0
  std::printf ("RESULT s=%d p=%d viols=%d\n", s, p, viols);
  return 0;
}
// { dg-output "RESULT s=5 p=0 viols=1" }
