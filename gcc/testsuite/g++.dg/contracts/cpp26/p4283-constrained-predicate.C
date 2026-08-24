// P4283 motivating case: a contract predicate that references names valid only
// when the constraint holds (here a member `.val` that exists only for class
// types satisfying HasVal).  When the constraint is not satisfied at
// instantiation the whole assertion is discarded and its predicate is never
// instantiated, so f(42) compiles even though `int` has no `.val`.
//
// Previously BUG-18: the requires-clause discarded the contract's *evaluation*
// but not its *instantiation* -- tsubst_contract checked the constraint only
// after substituting the predicate, so `x.val` was instantiated for T=int and
// rejected.  The constraint is now checked before the predicate is instantiated.
// See testing-gap-catalogue.md section 10.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

template <typename T> concept HasVal = requires(T t) { t.val; };

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++viol; }

template <typename T>
void f(T x) pre requires(HasVal<T>) (x.val > 0) { }

struct WithVal { int val; };

int main() {
  viol = 0;
  f(WithVal{1});   // constraint satisfied, predicate valid, holds
  if (viol != 0) __builtin_abort();
  f(WithVal{-1});  // satisfied, predicate false -> violation
  if (viol != 1) __builtin_abort();
  f(42);           // NOT satisfied -> assertion discarded; x.val never formed
  if (viol != 1) __builtin_abort();
}
