// A templated function first declared with a precondition whose lambda
// captures a parameter, and defined later without repeating the contract
// (which is allowed), checks that contract once instantiated.  The capture
// initializer names the first declaration's parameter, and has to be
// remapped to the definition's with the rest of the condition.
//
// Mirror: clang/test/Contracts/redecl-omit-contract-lambda.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

template <class T> struct S { int f (int x) pre ([&] { return x > 0; } ()); };
template <class T> int S<T>::f (int x) { return x; }

template <class T> struct U { int f (int x) pre ([y = x] { return y > 0; } ()); };
template <class T> int U<T>::f (int x) { return x; }

struct A { template <class T> T g (T x) pre ([x] { return x > 0; } ()); };
template <class T> T A::g (T x) { return x; }

template <class T> T fa (T x) pre ([=] { return x > 0; } ());
template <class T> T fa (T x) { return x; }

int main ()
{
  S<int> s; U<int> u; A a;
  s.f (1); u.f (1); a.g (1); fa (1);
  if (violations != 0)
    __builtin_abort ();
  s.f (-1); u.f (-1); a.g (-1); fa (-1);
  if (violations != 4)
    __builtin_abort ();
}
