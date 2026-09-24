// A PURE virtual's interface contract is actually evaluated.
//
// This is the run-time half of contract-instantiate-on-use.C.  A pure virtual
// has no definition to instantiate, so its interface contracts are substituted
// only because contract instantiation runs at odr-use -- and a
// predicate that was never substituted, or that was substituted and then never
// emitted, compiles exactly as quietly as a correct one.  Compiling is
// therefore not evidence; a violation count is.
//
// A compiler without on-odr-use substitution cannot compile this file at
// all, and one that has just gained it still needs to be shown that the
// checks it gained actually run.  This is that proof.
//
// The predicate is deliberately non-constant but NOT value-dependent: a
// dependent one reaches Clang's CodeGen still dependent and asserts there,
// pinned in open-bug-pure-virtual-dependent-predicate-codegen.C.
//
// Mirror of clang/test/Contracts/Runnable/pure-virtual-interface-contract.cpp.

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int viol = 0;
void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++viol;
}

static int limit = 0;
static int get_limit () { return limit; }

template <class T> struct A {
  // Pure, so this definition is never instantiated and the interface contract
  // exists only by way of on-use instantiation.
  virtual int get () const pre (get_limit () >= 0) = 0;
  virtual ~A () = default;
};

template <class T> struct D : A<T> {
  int get () const override { return 42; }
};

int
main ()
{
  D<int> d;
  A<int> &a = d;

  // Passing: the interface precondition holds.
  limit = 1;
  if (a.get () != 42)
    return 1;
  if (viol != 0)
    return 2;

  // Failing: the pure virtual's own interface precondition must fire, even
  // though the override carries no contracts and the pure declaration has no
  // body anywhere.
  limit = -1;
  if (a.get () != 42)
    return 3;
  if (viol != 1)
    return 4; // the interface contract was never evaluated

  // And again, to show it is evaluated per call rather than once.
  if (a.get () != 42)
    return 5;
  if (viol != 2)
    return 6;

  return 0;
}
