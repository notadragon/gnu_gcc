// P4405, DECISIONS.md E169 (14): -fcontract-analysis-skip-dead-calls,
// implied by -fcontracts-p4405, changes only the analysis evaluations at
// calls never evaluated: the run-time checks are the same as without it.
// The calls that are evaluated are checked (observe) and warned about by
// the analysis evaluation (diagnose); those never evaluated are neither.
//
// Mirror: clang/test/Contracts/p4405-analysis-dead-call-run.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405" }
// { dg-additional-options "-fcontract-analysis-semantic=diagnose" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

int g3 (int x) pre (x != 0);	// { dg-message "the predicate of the contract assertion" }
int g3 (int x) { return x ? 10 / x : 0; }

constexpr bool k = false;

__attribute__((noipa)) int zero () { return 0; }

int h1 () { return 1 ? 1 : g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
bool h2 () { return false && g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
int h3 (int y) { if (0) return g3 (0); return y; }	// { dg-bogus "(cannot be proven|always false)" }
int h5 () { return k ? g3 (0) : 2; }	// { dg-bogus "(cannot be proven|always false)" }
int live1 () { return 0 ? 2 : g3 (zero ()); }	// { dg-warning "cannot be proven" }
template <int N> int tn1 () { return N ? 1 : g3 (zero ()); }	// { dg-warning "cannot be proven" }

int main ()
{
  h1 ();
  h2 ();
  h3 (1);
  h5 ();
  if (violations != 0)
    __builtin_abort ();
  live1 ();
  tn1<0> ();
  tn1<1> ();
  if (violations != 2)
    __builtin_abort ();
}
