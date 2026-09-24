// P3100: implicit {stmt.return.flow.off} assertion configured to "enforce".
// On the fall-off the violation handler is invoked and then the program
// terminates (the enforce entry point is noreturn).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "enforce terminates after the handler runs" }
// { dg-output "GOT_VIOLATION(\n|\r\n|\r)" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation&)
{
  std::fputs ("GOT_VIOLATION\n", stderr);
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
