// P3100: the parent group ub:expr.add.out.of.bounds.known still selects both
// of its leaves, .subscript and .arithmetic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:observe" }
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
  int s = &a[vi] - a;		// caught -> 0
  int p = (a + vi) - a;		// caught -> 0
  std::printf ("RESULT s=%d p=%d viols=%d\n", s, p, viols);
  return 0;
}
// { dg-output "RESULT s=0 p=0 viols=2" }
