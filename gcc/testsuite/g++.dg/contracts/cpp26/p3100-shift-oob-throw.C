// P3100: shift-out-of-range with a throwing handler in a non-noexcept function
// -- propagates out and is caught by the caller, under observe and enforce.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type -Wno-shift-count-overflow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-shift-oob-throw-nonnoexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E { int tag; };
static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation&) {
  ++calls;
  throw E{7};
}

namespace obs_ns { int shl (int a, int b) { return a << b; } }   // observe
namespace enf_ns { int shl (int a, int b) { return a << b; } }   // enforce

int main () {
  int caught = 0;
  try { obs_ns::shl (1, 100); } catch (E& e) { if (e.tag == 7) ++caught; }
  try { enf_ns::shl (1, 100); } catch (E& e) { if (e.tag == 7) ++caught; }
  if (caught != 2) __builtin_abort ();
  if (calls != 2) __builtin_abort ();
}
