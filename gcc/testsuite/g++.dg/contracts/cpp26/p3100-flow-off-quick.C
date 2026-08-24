// P3100: implicit {stmt.return.flow.off} assertion configured to
// "quick_enforce".  On the fall-off the program terminates immediately without
// invoking the violation handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce terminates" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation&)
{
  // quick_enforce must NOT call the handler.
  std::fputs ("UNEXPECTED_HANDLER\n", stderr);
  std::fflush (stderr);
}

int f (int x)
{
  if (x > 0)
    return x;
  // x <= 0: control flows off the end ({stmt.return.flow.off}).
}

int main ()
{
  f (-1);
  return 0;
}
