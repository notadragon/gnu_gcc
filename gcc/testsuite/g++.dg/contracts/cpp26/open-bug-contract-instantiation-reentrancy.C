// Mirror of a CLANG-ONLY open bug: instantiating one function's contracts
// from inside another contract's predicate.
//
// Tracked as CLANG-13 in the llvm_llvm-project fork's open-issues/.  There
// Sema::PushContractScope asserts, because InstantiateFunctionContractsOnUse
// runs from MarkFunctionReferenced for every odr-use -- including the odr-use
// a contract predicate performs while that predicate is itself still being
// transformed -- and the contract scope is not re-entrant.
//
// Measured 2026-09-05: GCC compiles this clean, with -fcontracts and with
// -fcontracts-p3850.  So this file is NOT xfailed here; it is a plain
// expected-pass, and its job is to keep the two suites asking the same
// question.  If GCC ever grows the same defect, this turns into a hard
// failure rather than going unnoticed because only Clang had a test.
//
// Mirror: clang/test/Contracts/OpenBugs/contract-instantiation-reentrancy.cpp

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

template <class T>
struct S {
  bool ok () const pre (n >= 0) { return n >= 0; }
  int n = 0;
};

// The shape that crashes Clang: the predicate calls a contracted member, so
// transforming it re-enters contract instantiation.
template <class T>
int f (S<T> s) pre (s.ok ()) { return s.n; }

int
use_f ()
{
  return f (S<int>{});
}

// Control: a predicate that reaches no contracted function.
template <class T>
int g (S<T> s) pre (s.n >= 0) { return s.n; }

int
use_g ()
{
  return g (S<int>{});
}
