// P4283: constrained contract assertions in partial specializations and in
// nested templates.  The constraint is checked with the template arguments
// of the instantiation actually used:
//  * a partial specialization's member contract constrained on the
//    specialization's own parameter, and the primary template's member
//    with a different constraint, each kept or discarded independently;
//  * a member function template of a class template, constrained on both
//    the outer and the inner parameter, kept only when both hold;
//  * a member of a class template nested in a class template.
//
// Mirror: clang/test/Contracts/Runnable/p4283-partial-spec-nested.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <concepts>
#include <contracts>

static int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

static bool
counted (int n)
{
  bool ok = violations == n;
  violations = 0;
  return ok;
}

template <class T, class U>
struct P
{
  // Primary: kept when T is integral.
  void f (int x) pre requires (std::integral<T>) (x > 0) {}
};

template <class U>
struct P<double, U>
{
  // Partial specialization: kept when U is integral.
  void f (int x) pre requires (std::integral<U>) (x > 0) {}
};

template <class T>
struct Outer
{
  template <class U>
  void g (int x)
    pre requires (std::integral<T> && std::floating_point<U>) (x > 0)
  {}

  template <class U>
  struct Inner
  {
    void h (int x) pre requires (std::same_as<T, U>) (x > 0) {}
  };
};

int
main ()
{
  P<int, double> ().f (0);		// primary, int: kept
  if (!counted (1)) __builtin_abort ();
  P<float, int> ().f (0);		// primary, float: discarded
  if (!counted (0)) __builtin_abort ();
  P<double, int> ().f (0);		// specialization, U = int: kept
  if (!counted (1)) __builtin_abort ();
  P<double, double> ().f (0);		// specialization, U = double: discarded
  if (!counted (0)) __builtin_abort ();

  Outer<int> ().g<double> (0);		// both hold: kept
  if (!counted (1)) __builtin_abort ();
  Outer<int> ().g<int> (0);		// inner fails: discarded
  if (!counted (0)) __builtin_abort ();
  Outer<float> ().g<double> (0);	// outer fails: discarded
  if (!counted (0)) __builtin_abort ();

  Outer<int>::Inner<int> ().h (0);	// same: kept
  if (!counted (1)) __builtin_abort ();
  Outer<int>::Inner<long> ().h (0);	// different: discarded
  if (!counted (0)) __builtin_abort ();
}
