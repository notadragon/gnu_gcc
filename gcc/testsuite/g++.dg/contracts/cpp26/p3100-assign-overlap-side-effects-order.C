// P3100: the overlap check (ub:expr.assign.overlap) under "observe" keeps the
// meaning of an assignment whose operands have side effects: the right
// operand is evaluated before the left, so `a[k++] = a[k]' copies the
// element k names before the increment; each call in an operand is made
// once, and the right operand's first; the value read and the object stored
// to are the ones the operands designate, and the assignment's value is the
// stored one.  None of these overlap.
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
static int seq;
static int order[2];
int a[4] = {10, 20, 30, 40};
__attribute__((noipa)) int *left (int i) { order[0] = ++seq; return &a[i]; }
__attribute__((noipa)) int *right (int i) { order[1] = ++seq; return &a[i]; }
int main () {
  int k = 1;
  a[k++] = a[k];			// a[1] = a[1]: k read before the increment
  int r1 = a[1] * 100 + k;		// 2002
  int v = (*left (0) = *right (3));	// a[0] = a[3]
  std::printf ("RESULT r1=%d a0=%d v=%d right=%d left=%d viols=%d\n",
	       r1, a[0], v, order[1], order[0], viols);
  return 0;
}
// { dg-output "RESULT r1=2002 a0=40 v=40 right=1 left=2 viols=0" }
