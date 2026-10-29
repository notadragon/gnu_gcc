// D4405R0, "Proving Preconditions": a configuration entry with the analysis
// key gives the analysis evaluations of the "required" group's
// preconditions the prove semantic, at each call; f(0)'s predicate is
// always false once the argument is substituted, and f(1) is proven.  The
// entry says nothing about the runtime evaluations.  The paper's example,
// with its comments, and its configuration
// (p4405-paper-required-noconfig.C is the same without it).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-paper-required.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>  // `""group`

void f(int x) pre<"required"group>(x > 0);	// { dg-message "the predicate of the contract assertion" }

void g()
{
  f(1);  // OK	{ dg-bogus "always false|cannot be proven" }
  f(0);  // Error if the `"required"` group has the *prove* semantic	{ dg-error "contract predicate is always false" }
}
