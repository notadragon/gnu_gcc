// D4405R0, "Local Proofs": a postcondition with always_prove is proven
// against the function body (by the proof shadow, optimized).  The paper's
// example, with its comment.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdio>
#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

int g(bool b)
  post<always_prove>(r : r > 0)  // OK, proven	{ dg-bogus "(cannot be proven|always false)" }
{
  return b ? 17 : 34;
}

int main()
{
  std::printf("%d %d\n", g(true), g(false));
}

// { dg-output "17 34" }

// A -O0 build, at the default proof level, 2, gives the same diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
