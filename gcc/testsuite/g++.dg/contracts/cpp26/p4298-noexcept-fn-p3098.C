// General contracts/noexcept correctness (surfaced during D4298 review):
// same guarantee as p4298-noexcept-fn-inline.C, exercised through P3098's
// outlined postcondition-check function instead of an inline check.  Plain
// "enforce" semantic, not one of D4298's new semantics.
//
// Discrimination follows basic.contract.eval.p17-4.C: main() wraps the
// triggering call in try/catch(...).  If the outlined function's noexcept
// boundary correctly terminates, the installed terminate handler exits(0)
// before the exception ever reaches main's catch.  If the boundary were
// broken (e.g. the outlined function failed to inherit noexcept from the
// original function), the exception would propagate up, get caught by
// main's catch(...), and fall through to the "should not get here"
// __builtin_abort().
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <exception>
#include <cstdlib>

struct MyException {};

void my_term()
{
  try { throw; }
  catch (MyException) { std::exit(0); }
}

void handle_contract_violation(const std::contracts::contract_violation&)
{
  throw MyException{};
}

int f(int x) noexcept post(r: r > 0) { return x; }

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
