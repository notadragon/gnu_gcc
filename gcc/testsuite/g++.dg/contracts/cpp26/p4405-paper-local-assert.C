// D4405R0, "Local Proofs": contract_assert with always_diagnose warns when
// it cannot be proven, and with always_prove must be proven (here by the
// proof shadow, optimized).  Neither is ever checked at run time: f(-1)
// evaluates neither predicate.  The paper's example, with its comments.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdio>
#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

void f(int x)
{
  contract_assert<always_diagnose>(x > 0);  // OK, warns: cannot be proven	{ dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (x > 0) {
    contract_assert<always_prove>(x > 0);   // OK, proven	{ dg-bogus "cannot be proven" }
  }
}

int main()
{
  f(-1);  // Neither predicate is evaluated, so nothing is reported.
  std::puts("done");
}

// { dg-output "done" }

// A -O0 build, at the default proof level, 2, gives the same diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
