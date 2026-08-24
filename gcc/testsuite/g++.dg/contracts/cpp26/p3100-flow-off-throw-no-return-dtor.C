// P3100: when a throwing handler at flow-off-end unwinds out of the function,
// the (would-be) return object was never produced -- so it is not considered
// constructed and its destructor does not run.  Verified for observe and
// enforce.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-throw-nonnoexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E {};
struct Tracked {
  static int ctors, dtors;
  Tracked () { ++ctors; }
  Tracked (const Tracked&) { ++ctors; }
  Tracked (Tracked&&) { ++ctors; }
  ~Tracked () { ++dtors; }
};
int Tracked::ctors = 0;
int Tracked::dtors = 0;

void handle_contract_violation (const std::contracts::contract_violation&) {
  throw E{};
}

namespace obs_ns { Tracked f (int x) { if (x > 0) return Tracked{}; } }  // observe
namespace enf_ns { Tracked f (int x) { if (x > 0) return Tracked{}; } }  // enforce

int main () {
  int caught = 0;
  try { Tracked t = obs_ns::f (-1); (void) t; } catch (E&) { ++caught; }
  try { Tracked t = enf_ns::f (-1); (void) t; } catch (E&) { ++caught; }
  if (caught != 2) __builtin_abort ();
  // The return object was never constructed on the flow-off path, so neither
  // its constructor nor its destructor ran.
  if (Tracked::ctors != 0) __builtin_abort ();
  if (Tracked::dtors != 0) __builtin_abort ();
}
