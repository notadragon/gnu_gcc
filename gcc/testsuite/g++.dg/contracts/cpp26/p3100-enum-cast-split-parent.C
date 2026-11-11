// P3100: the parent group ub:expr.static.cast.enum.outside.range still
// selects both of its leaves (.integral and .floating): under "observe" an
// integral and a floating-point cast out of range are each reported, and
// each yields 0.
// Mirror: llvm_llvm-project clang/test/Contracts/Runnable/p3100-enum-cast-split.cpp (PARENT).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontract-group-evaluation-semantic=ub:expr.static.cast.enum.outside.range:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL [%s]\n", v.comment ());
  std::fflush (stdout);
}
enum E { A, B, C };   // [dcl.enum] value range [0,3]
__attribute__((noinline)) E from_int (int v) { return static_cast<E> (v); }
__attribute__((noinline)) E from_double (double v) { return static_cast<E> (v); }
int main () {
  int i = from_int (5);
  int d = from_double (5.0);
  std::printf ("RESULT i=%d d=%d\n", i, d);
  return 0;
}
// { dg-output "VIOL \\\[enumeration value out of range\\\](\n|\r\n|\r)VIOL \\\[enumeration value out of range\\\](\n|\r\n|\r)RESULT i=0 d=0" }
