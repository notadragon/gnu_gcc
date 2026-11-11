// P3100: a throwing violation handler at pointer arithmetic out of the bounds
// of an array of known bound (ub:expr.add.out.of.bounds.known.arithmetic)
// under "enforce": the site is in a function that may throw, so the exception
// propagates to the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL sem=%d\n", (int) v.semantic ()); std::fflush (stdout);
  throw Boom {};
}
int arr[3] = {10, 20, 30};
__attribute__((noinline)) int *add (int i) { return arr + i; }
int main () {
  try { std::printf ("RESULT=%d\n", (int) (add (4) - arr)); }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}
// { dg-output "VIOL sem=3(\n|\r\n|\r)CAUGHT(\n|\r\n|\r)SURVIVED" }
