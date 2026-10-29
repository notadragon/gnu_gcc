// P3100: an indirection past the end of an array of known bound
// (ub:expr.unary.dereference.invalid.known) configured to "enforce": the
// handler runs (kind=7, sem=3), then the program terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "enforce reports then terminates" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
int arr[3] = {10, 20, 30};
__attribute__((noinline)) int *addr (int i) { return &arr[i]; }
int main () { std::printf ("RESULT=%d\n", (int) (addr (3) - arr)); return 0; }
// { dg-output "VIOL kind=7 sem=3 comment=\\\[indirection past the end of an array\\\]" }
