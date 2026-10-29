// P3100: a misaligned placement new that stores nothing (ub:basic.align.object.alignment.nostore)
// configured to "observe": the handler runs (kind=7, sem=2) and execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
#include <new>
alignas (int) char buf[4 * sizeof (int)];
void *volatile where = buf + 1;
__attribute__((noinline)) int *make (void *p) { return new (p) int; }
int main () {
  int *q = make (where);
  std::printf ("RESULT=%d\n", (void *) q == where);
  return 0;
}
// { dg-output "VIOL kind=7 sem=2 comment=\\\[misaligned pointer passed to a non-allocating placement new\\\](\n|\r\n|\r)RESULT=1" }
