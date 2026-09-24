// P3100: integer division / remainder by zero ({expr.mul.div.by.zero}).
// ignore -> a defined (erroneous) 0 without executing the trapping division;
// observe -> the handler runs (reporting assertion_kind::implicit), then
// execution continues with the defined 0.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace cs = std::contracts;

static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation (const cs::contract_violation& v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit)
    all_implicit = false;
}

namespace ign_ns {                    // ignore
  int divi (int a, int b) { return a / b; }
  int modi (int a, int b) { return a % b; }
}
namespace obs_ns {                    // observe
  int divi (int a, int b) { return a / b; }
  int modi (int a, int b) { return a % b; }
}

int main () {
  // ignore: defined 0, no handler.
  if (ign_ns::divi (10, 0) != 0) __builtin_abort ();
  if (ign_ns::modi (10, 0) != 0) __builtin_abort ();
  if (calls != 0) __builtin_abort ();

  // observe: handler runs, continues with defined 0.
  if (obs_ns::divi (10, 0) != 0) __builtin_abort ();
  if (obs_ns::modi (10, 0) != 0) __builtin_abort ();
  if (calls != 2) __builtin_abort ();
  if (!all_implicit) __builtin_abort ();

  // Non-zero divisors are unaffected.
  if (ign_ns::divi (10, 2) != 5) __builtin_abort ();
  if (obs_ns::modi (10, 3) != 1) __builtin_abort ();
  if (calls != 2) __builtin_abort ();
}
