// P3100: a [[noreturn]] function that returns
// (ub:dcl.attr.noreturn.eventually.returns) and has a postcondition: the
// postcondition is evaluated first, at the return, and then the [[noreturn]]
// check reports and, under "observe", aborts.
// Mirror: llvm_llvm-project clang/test/Contracts/Runnable/p3100-noreturn-post.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "observe aborts after the handler" }
// { dg-prune-output "does return" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d comment=[%s]\n", (int) v.kind (), v.comment ());
  std::fflush (stdout);
}
[[noreturn]] __attribute__((noinline)) void nr (const int x) post (x < 0) {
  if (x < 0)
    throw x;
}
void (*volatile fp) (int) = nr;
int main () {
  fp (1);
  std::printf ("RESULT=1\n");
  return 0;
}
// { dg-output "VIOL kind=\[0-9\]+ comment=\\\[x < 0\\\](\n|\r\n|\r)VIOL kind=7 comment=\\\[a \\\[\\\[noreturn\\\]\\\] function returned\\\]" }
