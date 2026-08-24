// P3100: implicit {stmt.return.flow.off} assertion configured to "ignore".
// No violation handler is invoked, and the function returns a defined
// (erroneous) value -- zero for a scalar return type -- rather than leaving
// undefined behavior.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-ignore.json" }
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
  if (violations != 0) __builtin_abort ();  // ignore never calls the handler
  if (r != 0) __builtin_abort ();           // defined fallback value
}
