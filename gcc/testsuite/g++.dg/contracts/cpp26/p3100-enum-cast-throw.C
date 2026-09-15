// P3100: a THROWING violation handler at an out-of-range enum-cast under
// "enforce".  The cast site is in a non-noexcept function, so the handler's
// exception propagates out and is caught by the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-enum-cast-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL sem=%d\n", (int) v.semantic ()); std::fflush (stdout);
  throw Boom {};
}
enum E { A, B, C };
__attribute__((noinline)) E cast_it (int v) { return static_cast<E> (v); }
int main () {
  try { E e = cast_it (5); std::printf ("RESULT=%d\n", (int) e); }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}
// { dg-output "VIOL sem=3\r*\nCAUGHT\r*\nSURVIVED" }
