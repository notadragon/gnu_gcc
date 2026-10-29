// P3100: a null pointer passed to a non-allocating placement new (ub:expr.new.non.allocating.null)
// configured to "observe": the handler runs (kind=7, sem=2) and execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.new.non.allocating.null:observe" }
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
struct T { int x; };	// trivial: `new (p) T' stores nothing
void *volatile nowhere = nullptr;
__attribute__((noinline)) T *make (void *p) { return new (p) T; }
int main () {
  T *t = make (nowhere);
  std::printf ("RESULT=%d\n", t == nullptr);
  return 0;
}
// { dg-output "VIOL kind=7 sem=2 comment=\\\[null pointer passed to a non-allocating placement new\\\](\n|\r\n|\r)RESULT=1" }
