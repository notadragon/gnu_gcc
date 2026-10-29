// P3100: pointer arithmetic out of the bounds of an array of known bound
// (ub:expr.add.out.of.bounds.known.arithmetic) configured to
// "noexcept_enforce": the handler runs (kind=7, sem=7), then the program
// terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:noexcept_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce reports then terminates" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
int arr[3] = {10, 20, 30};
__attribute__((noinline)) int *add (int i) { return arr + i; }
int main () { std::printf ("RESULT=%d\n", (int) (add (4) - arr)); return 0; }
// { dg-output "VIOL kind=7 sem=7 comment=\\\[pointer arithmetic out of bounds\\\]" }
