// P3100: the enumeration-range check reports
// ub:expr.static.cast.enum.outside.range.integral for an integral source and
// .floating for a floating-point one, so the two are configured independently:
// here the integral cast is observed (the handler runs, and the result is 0)
// and the floating-point cast ignored (no handler, but the result is 0 too,
// where the default "assume" would leave 5).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontract-group-evaluation-semantic=ub:expr.static.cast.enum.outside.range.integral:observe,ub:expr.static.cast.enum.outside.range.floating:ignore" }
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
// { dg-output "VIOL \\\[enumeration value out of range\\\](\n|\r\n|\r)RESULT i=0 d=0" }
