// D4298: contract_violation::is_terminating() must report true for
// noexcept_enforce -- it aborts on a normally-returning handler exactly like
// enforce.  Regression: is_terminating() checked only enforce/quick_enforce and
// returned false for noexcept_enforce.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates on a normally-returning handler" }
// { dg-output "IS_TERMINATING(\n|\r\n|\r)" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.semantic () != std::contracts::evaluation_semantic::noexcept_enforce)
    return;                    // wrong semantic -> no marker -> test fails
  if (v.is_terminating ())
    {
      std::fputs ("IS_TERMINATING\n", stderr);
      std::fflush (stderr);
    }
  // Returns normally -> the ABI aborts (noexcept_enforce).
}

int f (int x) pre (x > 0) { return x; }

int main () { f (-1); return 0; }
