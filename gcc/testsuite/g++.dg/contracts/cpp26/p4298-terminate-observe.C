// D4298: a throwing handler at noexcept_observe terminates the program.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
#include <exception>
#include <cstdlib>

static bool terminate_called = false;

void handle_contract_violation(const std::contracts::contract_violation&)
{
  throw 1;
}

int f(int x) pre(x > 0) { return x; }

int main()
{
  std::set_terminate([]() { terminate_called = true; std::exit(0); });
  f(-1);
  __builtin_abort ();  // unreachable: f(-1) must terminate via the handler
}
