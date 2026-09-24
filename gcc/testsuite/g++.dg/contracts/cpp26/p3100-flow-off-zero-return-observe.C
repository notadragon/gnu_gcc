// P3100: flow-off under "observe" runs the handler and then continues with a
// byte-zeroed return object, for aggregate and by-reference return types.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-throw-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct Big { int a[8]; };
struct WithDtor { int a; long b; ~WithDtor () {} };

static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation&) {
  ++calls;
}

Big fbig (int x) { if (x > 0) return Big{{1, 2, 3, 4, 5, 6, 7, 8}}; }
WithDtor fw (int x) { if (x > 0) return WithDtor{1, 2}; }

int main () {
  Big g = fbig (-1);
  for (int i = 0; i < 8; ++i)
    if (g.a[i] != 0) __builtin_abort ();
  if (calls != 1) __builtin_abort ();

  WithDtor w = fw (-1);
  if (w.a != 0 || w.b != 0) __builtin_abort ();
  if (calls != 2) __builtin_abort ();
}
