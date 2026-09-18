// General contracts/noexcept correctness (surfaced during D4298 review):
// same guarantee as p4298-noexcept-fn-inline.C -- a throwing handler inside
// a noexcept function must terminate -- but exercised through the P3097
// virtual-override contract-check wrapper's codegen path instead of an
// inline check.  Plain "enforce" semantic, not one of D4298's new
// semantics: this is a general noexcept/contracts interaction, not
// specific to noexcept_enforce/noexcept_observe.
//
// Discrimination follows basic.contract.eval.p17-4.C: main() wraps the
// triggering call in try/catch(...).  If the wrapper's noexcept boundary
// correctly terminates, the installed terminate handler exits(0) before
// the exception ever reaches main's catch.  If the boundary were broken
// (e.g. the wrapper failed to inherit noexcept from the wrapped function),
// the exception would propagate up, get caught by main's catch(...), and
// fall through to the "should not get here" __builtin_abort().
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=enforce" }
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

struct base {
  virtual int f(int x) noexcept pre(x > 0) { return x; }
};
struct derived : base {
  int f(int x) noexcept override { return x; }
};

// A call through a base reference cannot be resolved to a fixed dynamic
// type by the front end, so it goes through P3097's virtual-dispatch
// wrapper (see p3097-basic.C).
int call_virtually(base& b, int x) { return b.f(x); }

int main()
{
  std::set_terminate(my_term);
  try
    {
      derived d;
      call_virtually(d, -1);
    }
  catch (...)
    {
    }
  // We should not get here.
  __builtin_abort ();
}
