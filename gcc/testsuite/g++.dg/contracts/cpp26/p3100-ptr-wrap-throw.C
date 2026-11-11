// P3100: a throwing violation handler at pointer arithmetic that wraps around
// the address space (ub:expr.add.out.of.bounds.pointer.arithmetic) under
// "enforce": the site is in a function that may throw, so the exception
// propagates to the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL sem=%d\n", (int) v.semantic ()); std::fflush (stdout);
  throw Boom {};
}
#include <cstdint>
__attribute__((noinline)) char *add (char *p, long i) { return p + i; }
char *top = reinterpret_cast<char *> (UINTPTR_MAX - 1);
int main () {
  try { std::printf ("RESULT=%d\n", (int) (add (top, 4) - top)); }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}
// { dg-output "VIOL sem=3(\n|\r\n|\r)CAUGHT(\n|\r\n|\r)SURVIVED" }
