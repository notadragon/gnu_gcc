// P3100: a scalar assignment from an overlapping object
// (ub:expr.assign.overlap) whose operands have side effects is checked under
// "observe", each operand evaluated once: an operand whose address
// computation has side effects is evaluated through its saved address.  An
// index incremented in the left operand, a volatile source and a source
// reached through a call are caught; the same assignments between disjoint
// objects are not.
// Mirror: llvm_llvm-project clang/test/Contracts/Runnable/p3100-assign-overlap-side-effects.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.assign.overlap:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation (const cs::contract_violation& v) {
  if (v.kind () == cs::assertion_kind::implicit)
    ++viols;
}
union U { int i; short s; };
union V { int i; volatile short s; };
static int calls;
__attribute__((noipa)) U *get (U *p) { ++calls; return p; }

__attribute__((noipa)) int incremented (U *p, int k) {
  p[k++].i = p[k].s;
  return k;
}
__attribute__((noipa)) void volatile_src (V *p) { p->i = p->s; }
__attribute__((noipa)) void through_call (U *p, U *q) { p->i = get (q)->s; }

int main () {
  U u[2] = {};
  V v = {};
  through_call (&u[0], &u[1]);
  std::printf ("clean=%d calls=%d\n", viols, calls);
  int k = incremented (u, 0);
  volatile_src (&v);
  through_call (&u[0], &u[0]);
  std::printf ("caught=%d calls=%d k=%d\n", viols, calls, k);
  return 0;
}
// { dg-output "clean=0 calls=1(\n|\r\n|\r)caught=3 calls=2 k=1" }
