// P3100: shift by a negative amount or an amount >= the promoted left operand's
// width is core-language UB ({expr.shift.neg.and.width}).  ignore -> defined 0
// without shifting; observe -> handler (assertion_kind::implicit) then 0.
// Covers << and >>.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type -Wno-shift-count-overflow -Wno-shift-count-negative" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-shift-oob.json" }
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
  int shl (int a, int b) { return a << b; }
  int shr (int a, int b) { return a >> b; }
}
namespace obs_ns {                    // observe
  int shl (int a, int b) { return a << b; }
  int shr (int a, int b) { return a >> b; }
}

int main () {
  // ignore: out-of-range shift -> defined 0, no handler.
  if (ign_ns::shl (1, 100) != 0) __builtin_abort ();   // count >= width
  if (ign_ns::shl (1, -1) != 0) __builtin_abort ();    // negative count
  if (ign_ns::shr (256, 100) != 0) __builtin_abort ();
  if (calls != 0) __builtin_abort ();

  // observe: handler runs, then continue with 0.
  if (obs_ns::shl (1, 100) != 0) __builtin_abort ();
  if (obs_ns::shr (256, -3) != 0) __builtin_abort ();
  if (calls != 2) __builtin_abort ();
  if (!all_implicit) __builtin_abort ();

  // In-range shifts are unaffected.
  if (ign_ns::shl (1, 4) != 16) __builtin_abort ();
  if (obs_ns::shr (256, 2) != 64) __builtin_abort ();
  if (calls != 2) __builtin_abort ();
}
