// P3100: flowing off the end of the try-block body of a *function-try-block*
// is the flow-off-end violation ({stmt.return.flow.off}), and the violation
// occurs inside the function-body scope -- so the function-try-block's own
// handler can catch a throwing observe/enforce handler.  Under ignore the
// try-body fall-off produces a defined return instead (no handler runs).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-fntryblock.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E {};
static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation&) {
  ++calls;
  throw E{};
}

namespace obs_ns {                                     // observe
  int f (int x) try { if (x > 0) return x; } catch (E&) { return -1; }
}
namespace enf_ns {                                     // enforce
  int f (int x) try { if (x > 0) return x; } catch (E&) { return -1; }
}
namespace ign_ns {                                     // ignore
  int f (int x) try { if (x > 0) return x; } catch (E&) { return -1; }
}

int main () {
  // observe/enforce: the try body falls off, the handler throws, and the
  // function-try-block's own catch catches it and returns -1.
  if (obs_ns::f (-1) != -1) __builtin_abort ();
  if (enf_ns::f (-1) != -1) __builtin_abort ();
  if (calls != 2) __builtin_abort ();

  // ignore: the try body fall-off yields a defined return (0); no handler runs
  // and the catch is not entered.
  if (ign_ns::f (-1) != 0) __builtin_abort ();
  if (calls != 2) __builtin_abort ();

  // Normal returns are unaffected.
  if (obs_ns::f (7) != 7) __builtin_abort ();
  if (enf_ns::f (7) != 7) __builtin_abort ();
  if (ign_ns::f (7) != 7) __builtin_abort ();
  if (calls != 2) __builtin_abort ();
}
