// P3100: with both the bounds checks (ub:expr.add.out.of.bounds.known: the
// subscript and arithmetic leaves) and the past-the-end indirection check
// (ub:expr.unary.dereference.invalid.known) observed, *(a + N + 2) is
// reported once, by the arithmetic check, which moves the pointer to the
// first element before the indirection check sees it; *(a + N) is reported
// by the indirection check alone.  (p3100-invalid-known-with-subscript.C has
// the subscript forms.)
// Mirror: llvm_llvm-project clang/test/Contracts/Runnable/p3100-invalid-known-with-subscript.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:observe,ub:expr.unary.dereference.invalid.known:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL [%s]\n", v.comment ());
  std::fflush (stdout);
}
int a[3] = {10, 20, 30};
volatile int vi;
int main () {
  vi = 5; int r3 = *(a + vi);
  std::printf ("R3=%d\n", r3);
  vi = 3; int r4 = *(a + vi);
  std::printf ("R4=%d\n", r4);
  return 0;
}
// { dg-output "VIOL \\\[pointer arithmetic out of bounds\\\](\n|\r\n|\r)R3=10(\n|\r\n|\r)VIOL \\\[indirection past the end of an array\\\](\n|\r\n|\r)R4=10" }
