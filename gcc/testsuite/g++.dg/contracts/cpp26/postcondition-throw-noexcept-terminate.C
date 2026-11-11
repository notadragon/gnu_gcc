// The other half of [basic.contract.eval]/17's note: "If the function has a
// non-throwing exception specification, the function std::terminate is
// invoked ([except.terminate])."
//
// The handler throws out of a postcondition on a noexcept function.  Since
// the behavior is as if the function body exits via that exception, and the
// function is noexcept, the program terminates -- the try/catch in the body
// does not run, and neither does the one in main.
//
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3850 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "throwing handler on a noexcept function terminates" }

#include <contracts>
#include <cstdio>
#include <cstdlib>

struct Bail
{
};

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  std::fputs ("contract handler ran\n", stderr);
  throw Bail{};
}

static int
f () noexcept post (r : r > 0)
{
  try
    {
      return -1;
    }
  catch (...)
    {
      std::_Exit (0); // must not run
    }
}

int
main ()
{
  try
    {
      f ();
    }
  catch (const Bail &)
    {
      std::_Exit (0); // must not run either
    }
  return 0;
}

// A failed check exits normally, which dg-shouldfail reports as a failure;
// the handler must have run.
// { dg-output "contract handler ran" }
