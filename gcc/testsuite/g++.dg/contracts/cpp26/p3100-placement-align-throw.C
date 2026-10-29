// P3100: a misaligned placement new that stores nothing (ub:basic.align.object.alignment.nostore)
// a throwing violation handler under "enforce": the exception propagates to the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL sem=%d\n", (int) v.semantic ()); std::fflush (stdout);
  throw Boom {};
}
#include <new>
alignas (int) char buf[4 * sizeof (int)];
void *volatile where = buf + 1;
__attribute__((noinline)) int *make (void *p) { return new (p) int; }
int main () {
  try {
  int *q = make (where);
  std::printf ("RESULT=%d\n", (void *) q == where);
  }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}
// { dg-output "VIOL sem=3(\n|\r\n|\r)CAUGHT(\n|\r\n|\r)SURVIVED" }
