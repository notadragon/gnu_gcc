// P3100 x D4298: implicit {stmt.return.flow.off} assertion configured to the
// non-throwing "noexcept_observe" semantic (requires -fcontracts-p4298).  The
// handler runs and execution continues, returning a defined (erroneous) value.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-noexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++violations;
}

int f (int x)
{
  if (x > 0)
    return x;
  // x <= 0: control flows off the end ({stmt.return.flow.off}).
}

int main ()
{
  int r = f (-1);
  if (violations != 1) __builtin_abort ();
  if (r != 0) __builtin_abort ();
}
