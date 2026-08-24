// P3595 dynamic selection through a template instantiation whose contract also
// carries a requires-clause (P4283).  A satisfied, non-dependent requires-clause
// keeps the contract, and the instantiated contract must still get its dynamic
// descriptor populated -- otherwise it falls back to the static default
// ("ignore") and the selector is never consulted.  Default is "ignore"; the
// runtime selector returns "observe", so the failing call is counted once.
// (Clang mirror: clang/test/Contracts/Runnable/p3595-dynamic-template-requires.cpp)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4283" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-template-requires.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

evaluation_semantic p3595_tpl_req_sel() { return evaluation_semantic::observe; }

template <class T>
concept Sized = sizeof(T) > 0;

template <class T>
void f(const T x) pre requires(Sized<T>) (x > 0) { }

int main() {
  f<int>(-1);                    // observe -> handler called, continue
  if (violations != 1) __builtin_abort();
  f<int>(1);                     // no violation
  if (violations != 1) __builtin_abort();
}
