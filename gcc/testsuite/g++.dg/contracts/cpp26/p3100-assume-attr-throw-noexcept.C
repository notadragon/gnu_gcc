// P3100: a throwing [[assume]] violation handler under observe in a noexcept
// function -- the exception reaches the noexcept boundary and terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-assume-attr-throw-noexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "throwing handler crosses noexcept boundary -> terminate" }

#include <contracts>
#include <cstdio>
#include <cstdlib>

struct E {};
void handle_contract_violation (const std::contracts::contract_violation &) {
  std::fputs ("contract handler ran\n", stderr);
  throw E{};
}

namespace obs_ns { int f (int x) noexcept { [[assume (x > 0)]]; return x; } }

int main () { return obs_ns::f (-1); }

// A failed check exits normally, which dg-shouldfail reports as a failure;
// the handler must have run.
// { dg-output "contract handler ran" }
