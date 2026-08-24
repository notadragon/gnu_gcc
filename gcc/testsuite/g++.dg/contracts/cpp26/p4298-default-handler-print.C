// D4298: the built-in default violation handler names the noexcept semantics.
// Regression: its semantic printer switched only on enforce/observe, so a
// noexcept_observe/noexcept_enforce violation printed "unknown(6)"/"unknown(7)".
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-output "semantic: noexcept_observe" }

#include <contracts>

// No user handle_contract_violation -> the default handler runs and prints;
// noexcept_observe continues after it.
int f (int x) pre (x > 0) { return x; }

int main () { f (-1); return 0; }
