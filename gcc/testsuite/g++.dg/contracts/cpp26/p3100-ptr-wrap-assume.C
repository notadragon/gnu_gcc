// P3100: pointer arithmetic that wraps around the address space
// (ub:expr.add.out.of.bounds.pointer.arithmetic) left at the default "assume":
// no check is emitted.  The wrapped pointer is formed.
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
#include <cstdint>
__attribute__((noinline)) char *add (char *p, long i) { return p + i; }
char *top = reinterpret_cast<char *> (UINTPTR_MAX - 1);
int main () { std::printf ("RESULT=%d\n", (int) (add (top, 4) - top)); return 0; }
// { dg-output "RESULT=4" }
