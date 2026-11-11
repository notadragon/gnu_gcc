// P3100: a null pointer passed to a non-allocating placement new (ub:expr.new.non.allocating.null)
// a throwing violation handler under "enforce": the exception propagates to the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.new.non.allocating.null:enforce" }
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
struct T { int x; };	// trivial: `new (p) T' stores nothing
void *volatile nowhere = nullptr;
__attribute__((noinline)) T *make (void *p) { return new (p) T; }
int main () {
  try {
  T *t = make (nowhere);
  std::printf ("RESULT=%d\n", t == nullptr);
  }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}
// { dg-output "VIOL sem=3(\n|\r\n|\r)CAUGHT(\n|\r\n|\r)SURVIVED" }
