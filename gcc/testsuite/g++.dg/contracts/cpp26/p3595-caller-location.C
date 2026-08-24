// P3595: caller-side semantic resolved per call site.
// The callee-side pre() is ignored; each call site gets its own wrapper whose
// caller-side semantic depends on the call-site location.  The call on line 20
// matches the "caller location 20-20 -> observe" rule; the call on line 21
// falls through to "caller -> ignore".  So the two call sites must diverge.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-location.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int fired = 0;
void handle_contract_violation (const std::contracts::contract_violation &) {
  ++fired;
}

int f (int x) pre(x > 0) { return x; }

int call_hot () { return f (-1); }
int call_cold () { return f (-1); }

int main () {
  call_hot ();                          // f() called from line 20
  int after_hot = fired;
  call_cold ();                         // f() called from line 21
  // hot call observes (fired increments); cold call is ignored (no change).
  if (after_hot != 1 || fired != 1) __builtin_abort ();
  return 0;
}
