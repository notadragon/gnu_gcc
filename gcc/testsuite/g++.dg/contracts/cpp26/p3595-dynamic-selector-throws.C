// P3595: the dynamic selector is an ordinary function, so an exception from
// it unwinds like any other: through the handlers and the cleanups of the
// function evaluating the contract.  Both places a selector is called: the
// dispatch of a contract_assert, and the selection a postcondition with
// captures makes where the captures are initialized.
// p3595-dynamic-selector-throws-O2.C runs this at -O2,
// p3595-dynamic-selector-throws-outlined.C with -fcontract-checks-outlined.
//
// Mirror: in the llvm_llvm-project fork,
// clang/test/Contracts/Runnable/p3595-dynamic-selector-throws.cpp
// (CLANG-835).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-selector-throws.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int dtors, caught;

struct D
{
  ~D () { ++dtors; }
};

evaluation_semantic
p3595_throwing_sel ()
{
  throw 1;
}

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  std::abort ();	// the selector throws before any check
}

// A handler in the same function.
void
catches_itself (int x)
{
  D d;
  try
    {
      contract_assert (x > 0);
    }
  catch (int)
    {
      ++caught;
    }
}

// No handler: the local's destructor runs on the way out.
void
unwinds (int x)
{
  D d;
  contract_assert (x > 0);
}

// The selection made where the captures are initialized.
int
captures (const int x) post [d = D ()] (x > 0)
{
  D local;
  return x;
}

int
main ()
{
  catches_itself (1);
  if (caught != 1 || dtors != 1)
    std::abort ();
  try
    {
      unwinds (1);
      std::abort ();
    }
  catch (int)
    {
    }
  if (dtors != 2)
    std::abort ();
  try
    {
      captures (1);
      std::abort ();
    }
  catch (int)
    {
    }
  if (dtors != 2)	// neither the capture nor the local was constructed
    std::abort ();
}
