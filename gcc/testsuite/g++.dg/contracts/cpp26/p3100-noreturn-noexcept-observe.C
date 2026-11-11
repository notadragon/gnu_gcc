// P3100: a [[noreturn]] function that returns (ub:dcl.attr.noreturn.eventually.returns)
// configured to "noexcept_observe": the handler runs (kind=7, sem=6), then, with no valid continuation, the program aborts.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_observe reports then aborts" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
// { dg-prune-output "does return" }
[[noreturn]] __attribute__((noinline)) void nr (int x) { if (x < 0) throw x; }
void (*volatile fp) (int) = nr;	// the caller does not know nr is noreturn
int main () {
  fp (1);
  std::printf ("RESULT=1\n");
  return 0;
}
// { dg-output "VIOL kind=7 sem=6 comment=\\\[a \\\[\\\[noreturn\\\]\\\] function returned\\\]" }
