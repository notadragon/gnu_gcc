// D4405R0, "Analyzing Assertions With Restricted Semantics": built to ignore
// contract assertions, the terminating precondition is still enforced
// (ignore is not in its allowed set), and a configured analysis semantic
// of diagnose, which terminating does not allow, falls back to ignore, so
// nothing is diagnosed.  The paper's illustration
// (p4405-paper-analysis-terminating.C configures prove).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "enforce terminates after the violation" }

#include <contracts>  // `terminating`, `contract_violation`
#include <cstdio>     // `std::printf`, `std::fflush`

using std::contracts::labels::terminating;

// A violation handler that reports each violation; with *enforce* the
// program then terminates.
void handle_contract_violation(const std::contracts::contract_violation& v)
{
  std::printf("violation: %s\n", v.comment());
  std::fflush(stdout);
}

int table[101];

int lookup(int p)
  pre<terminating>(p >= 0 && p <= 100)	// { dg-bogus "cannot be proven" }
{
  return table[p];
}

int lookup_clamped(int v)
{
  if (v < 0)
    v = 0;
  if (v > 100)
    v = 100;
  return lookup(v);  // OK, proven	{ dg-bogus "cannot be proven" }
}

int lookup_raw(int v)
{
  return lookup(v);  // Error if proven, otherwise enforced	{ dg-bogus "cannot be proven" }
}

int main()
{
  return lookup_clamped(1000) + lookup_raw(1000);
}

// { dg-output "violation: p >= 0 && p <= 100" }
