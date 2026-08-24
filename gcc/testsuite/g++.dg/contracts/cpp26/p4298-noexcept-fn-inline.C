// General contracts/noexcept correctness (surfaced during D4298 review):
// a contract-violation handler that throws inside a noexcept function must
// terminate the program, regardless of the contract's own evaluation
// semantic -- here plain "enforce", not one of D4298's noexcept_enforce/
// noexcept_observe semantics -- because the enclosing function is itself
// noexcept and cannot let the exception escape.  Inline (non-outlined,
// non-wrapper) check codegen.
//
// Discrimination follows basic.contract.eval.p17-4.C: main() wraps the
// triggering call in try/catch(...).  If the noexcept boundary correctly
// terminates, the installed terminate handler exits(0) before the
// exception ever reaches main's catch.  If the boundary were broken, the
// exception would propagate up, get caught by main's catch(...), and fall
// through to the "should not get here" __builtin_abort().
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <exception>
#include <cstdlib>

struct MyException {};

// Test that there is an active exception when we reach the terminate handler.
void my_term()
{
  try { throw; }
  catch (MyException) { std::exit(0); }
}

void handle_contract_violation(const std::contracts::contract_violation&)
{
  throw MyException{};
}

int f(int x) noexcept pre(x > 0) { return x; }

int main()
{
  std::set_terminate(my_term);
  try
    {
      f(-1);
    }
  catch (...)
    {
    }
  // We should not get here.
  __builtin_abort ();
}
