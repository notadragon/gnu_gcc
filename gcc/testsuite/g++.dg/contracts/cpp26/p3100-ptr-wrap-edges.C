// P3100: pointer arithmetic that wraps around the address space
// (ub:expr.add.out.of.bounds.pointer.arithmetic) under "observe".  Adding past
// the top, subtracting below zero, p += i and the address of p[i] are caught
// and leave the pointer unmoved; arithmetic that does not wrap, in either
// direction, is not.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdint>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation (const cs::contract_violation& v) {
  if (v.kind () == cs::assertion_kind::implicit)
    ++viols;
}
volatile long vi;
int main () {
  char *top = reinterpret_cast<char *> (UINTPTR_MAX - 1);
  char *low = reinterpret_cast<char *> (uintptr_t (2));
  long r = 0;
  vi = 4; r += (top + vi) - top;	// caught -> 0
  vi = 4; r += (low - vi) - low;	// caught -> 0
  vi = -4; r += (low + vi) - low;	// caught -> 0
  vi = 1; r += (top + vi) - top;	// 1: fine
  vi = 2; r += (low - vi) - low;	// -2: fine
  char *p = top;
  vi = 8; p += vi; r += p - top;	// caught -> 0
  int *ip = reinterpret_cast<int *> (UINTPTR_MAX - 7);
  vi = 4; r += &ip[vi] - ip;		// caught -> 0
  char buf[8];
  vi = 3; r += (buf + vi) - buf;	// 3: fine
  std::printf ("RESULT r=%ld viols=%d\n", r, viols);
  return 0;
}
// { dg-output "RESULT r=2 viols=5" }
