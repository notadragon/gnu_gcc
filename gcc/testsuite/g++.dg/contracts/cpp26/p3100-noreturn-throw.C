// P3100: a [[noreturn]] function that returns (ub:dcl.attr.noreturn.eventually.returns)
// a throwing violation handler under "enforce": the exception propagates to the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL sem=%d\n", (int) v.semantic ()); std::fflush (stdout);
  throw Boom {};
}
// { dg-prune-output "does return" }
[[noreturn]] __attribute__((noinline)) void nr (int x) { if (x < 0) throw x; }
void (*volatile fp) (int) = nr;	// the caller does not know nr is noreturn
int main () {
  try {
  fp (1);
  std::printf ("RESULT=1\n");
  }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}
// { dg-output "VIOL sem=3(\n|\r\n|\r)CAUGHT(\n|\r\n|\r)SURVIVED" }
