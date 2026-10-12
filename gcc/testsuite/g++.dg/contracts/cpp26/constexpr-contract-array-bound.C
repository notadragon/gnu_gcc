// An array bound whose only non-constant part is a contract predicate is a
// constant bound: the violation is diagnosed and the array is not a VLA.  A
// bound that is not constant apart from its predicate is a VLA, and its
// predicate is left to run time.  (GCC-620: the first diagnostic was emitted
// twice, which a dg-error cannot see; verify.sh tracks it.)
//
// Mirror: clang/test/Contracts/constexpr-contract-array-bound.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fsyntax-only -Wno-vla" }
// -Wno-vla: use_vla's array is a VLA by design, which the testsuite's
// -pedantic-errors would reject; and after the contract error in use, GCC
// recovers by treating that bound as non-constant too.  That the bound is
// constant is shown under observe, where sizeof is usable (see the
// contract test matrix's notional group).

bool rt ();

constexpr int
f (const int &v)
  pre (false) // { dg-error "contract predicate is false" }
{
  return 1;
}

int
use (int p)
{
  char a[f (p)];
  return sizeof a;
}

constexpr int
h (const int &v)
  pre (false)
{
  return rt () ? 1 : 1;
}

int
use_vla (int p)
{
  char a[h (p)];
  return sizeof a;
}
