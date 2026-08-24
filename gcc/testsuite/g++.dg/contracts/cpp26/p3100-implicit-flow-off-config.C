// P3100 x P3595: an implicit contract assertion for falling off the end of a
// value-returning function ({stmt.return.flow.off}), configured via kind
// "implicit" to the "observe" semantic.  On the fall-off the violation handler
// is invoked and execution then CONTINUES, returning a defined (erroneous)
// value -- zero for a scalar return type.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-implicit-flow-off-config.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation& v)
{
  ++violations;
  // P3100: an implicit contract assertion reports assertion_kind::implicit.
  if (v.kind () != std::contracts::assertion_kind::implicit)
    __builtin_abort ();
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
  if (violations != 1) __builtin_abort ();  // handler ran exactly once
  if (r != 0) __builtin_abort ();           // observe continues, defined 0
}
