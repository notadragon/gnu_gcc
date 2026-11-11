// P3100: divide-by-zero with a THROWING handler in a NOEXCEPT function -- the
// exception reaches the noexcept boundary and calls std::terminate.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "throwing handler in a noexcept function terminates" }

#include <contracts>
#include <cstdio>
#include <cstdlib>

struct E {};
void handle_contract_violation (const std::contracts::contract_violation&) {
  std::fputs ("contract handler ran\n", stderr);
  throw E{};
}

int divi (int a, int b) noexcept { return a / b; }   // observe

int main () { return divi (1, 0); }

// A failed check exits normally, which dg-shouldfail reports as a failure;
// the handler must have run.
// { dg-output "contract handler ran" }
