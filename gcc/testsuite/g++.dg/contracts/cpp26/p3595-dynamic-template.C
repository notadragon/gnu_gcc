// P3595 dynamic selection through a template instantiation.  The contract lives
// in a function template, so it reaches codegen via template instantiation
// rather than the primary parse path; the dynamic descriptor must be populated
// on that path too, or the instantiated contract falls back to the static
// (compile-time) default -- here "ignore" -- and the selector is never
// consulted.  Default is "ignore"; the runtime selector returns "observe", so
// the failing call is counted once and execution continues.
// (Clang mirror: clang/test/Contracts/Runnable/p3595-dynamic-template.cpp)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-template.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

evaluation_semantic p3595_tpl_sel() { return evaluation_semantic::observe; }

template <class T>
void f(const T x) pre(x > 0) { }

int main() {
  f<int>(-1);                    // observe -> handler called, continue
  if (violations != 1) __builtin_abort();
  f<int>(1);                     // no violation
  if (violations != 1) __builtin_abort();
}
