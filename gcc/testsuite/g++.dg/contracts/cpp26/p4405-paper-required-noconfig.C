// D4405R0, "Proving Preconditions": p4405-paper-required.C without its
// configuration -- the analysis evaluations are ignore, and nothing is
// diagnosed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>  // `""group`

void f(int x) pre<"required"group>(x > 0);

void g()
{
  f(1);  // OK	{ dg-bogus "always false|cannot be proven" }
  f(0);  // Error if the `"required"` group has the *prove* semantic	{ dg-bogus "always false|cannot be proven" }
}
