// P3100: with both the subscript check
// (ub:expr.add.out.of.bounds.known.subscript) and the past-the-end indirection
// check (ub:expr.unary.dereference.invalid.known) observed, &a[N + 2] is
// reported once, by the subscript check, which redirects the index before the
// indirection check sees it; &a[N] is reported by the indirection check alone.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.subscript:observe,ub:expr.unary.dereference.invalid.known:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL [%s]\n", v.comment ());
  std::fflush (stdout);
}
int a[3];
volatile int vi;
int main () {
  vi = 5; int r1 = &a[vi] - a;
  std::printf ("R1=%d\n", r1);
  vi = 3; int r2 = &a[vi] - a;
  std::printf ("R2=%d\n", r2);
  return 0;
}
// { dg-output "VIOL \\\[array subscript out of bounds\\\](\n|\r\n|\r)R1=0(\n|\r\n|\r)VIOL \\\[indirection past the end of an array\\\](\n|\r\n|\r)R2=0" }
