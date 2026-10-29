// P3100: pointer arithmetic that wraps around the address space
// (ub:expr.add.out.of.bounds.pointer.arithmetic) configured to "enforce": the
// handler runs (kind=7, sem=3), then the program terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:enforce" }
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
#include <cstdint>
__attribute__((noinline)) char *add (char *p, long i) { return p + i; }
char *top = reinterpret_cast<char *> (UINTPTR_MAX - 1);
int main () { std::printf ("RESULT=%d\n", (int) (add (top, 4) - top)); return 0; }
// { dg-output "VIOL kind=7 sem=3 comment=\\\[pointer arithmetic overflow\\\]" }
